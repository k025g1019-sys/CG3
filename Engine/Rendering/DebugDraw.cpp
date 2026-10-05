#include "Engine/Rendering/DebugDraw.h"

#include <cassert>
#include <cmath>
#include <cstring>

#include "Engine/Core/DirectXCore.h"
#include "Engine/Diagnostics/Log.h"
#include "Engine/Graphics/GpuResource.h"
#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Graphics/ShaderCompiler.h"
#include "Engine/Math/Curve.h"
#include "Engine/Math/Matrix4x4.h"

using Microsoft::WRL::ComPtr;

namespace Engine {

namespace {

constexpr float kPi = 3.14159265f;

// 球を描くときの緯度・経度の分割数
constexpr uint32_t kSphereSubdivision = 12;

// 曲線を折れ線で近似するときの分割数
constexpr uint32_t kCurveSubdivision = 32;

// 曲線の制御点に描く十字の大きさ（半分の長さ）
constexpr float kControlPointMarkSize = 0.08f;

// 与えた向きに垂直な単位ベクトルを1つ求める（My_Math の Plane::Perpendicular と同じ考え方）
Vector3 Perpendicular(const Vector3& vector) {
	if (std::fabs(vector.x) > std::fabs(vector.y)) {
		return Normalize({ -vector.y, vector.x, 0.0f });
	}
	return Normalize({ 0.0f, -vector.z, vector.y });
}

}  // namespace

// ===================== 形の描画（線を積むだけ）=====================

void DebugDraw::DrawLine(const Vector3& start, const Vector3& end, const Vector4& color) {
	GetInstance()->AddLine(start, end, color);
}

void DebugDraw::DrawSegment(const Segment3D& segment, const Vector4& color) {
	DrawLine(segment.start, segment.end, color);
}

void DebugDraw::DrawSphere(const Sphere& sphere, const Vector4& color) {
	const float kLonEvery = 2.0f * kPi / float(kSphereSubdivision);  // 経度分割1つ分の角度
	const float kLatEvery = kPi / float(kSphereSubdivision);         // 緯度分割1つ分の角度

	// 緯度の方向に分割 -π/2 ~ π/2
	for (uint32_t latIndex = 0; latIndex < kSphereSubdivision; ++latIndex) {
		const float lat = -kPi / 2.0f + kLatEvery * float(latIndex);
		// 経度の方向に分割 0 ~ 2π
		for (uint32_t lonIndex = 0; lonIndex < kSphereSubdivision; ++lonIndex) {
			const float lon = kLonEvery * float(lonIndex);

			// 点a（現在の緯度・経度）、b（緯度を1つ進めた点）、c（経度を1つ進めた点）
			const Vector3 a = sphere.center + Vector3{
				std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon) } * sphere.radius;
			const Vector3 b = sphere.center + Vector3{
				std::cos(lat + kLatEvery) * std::cos(lon), std::sin(lat + kLatEvery), std::cos(lat + kLatEvery) * std::sin(lon) } * sphere.radius;
			const Vector3 c = sphere.center + Vector3{
				std::cos(lat) * std::cos(lon + kLonEvery), std::sin(lat), std::cos(lat) * std::sin(lon + kLonEvery) } * sphere.radius;

			DrawLine(a, b, color);
			DrawLine(a, c, color);
		}
	}
}

void DebugDraw::DrawAABB(const AABB3D& aabb, const Vector4& color) {
	const Vector3 center = (aabb.min + aabb.max) * 0.5f;
	const Vector3 half = (aabb.max - aabb.min) * 0.5f;
	OBB3D obb{};
	obb.center = center;
	obb.orientations[0] = { 1.0f, 0.0f, 0.0f };
	obb.orientations[1] = { 0.0f, 1.0f, 0.0f };
	obb.orientations[2] = { 0.0f, 0.0f, 1.0f };
	obb.size = half;
	DrawOBB(obb, color);
}

void DebugDraw::DrawOBB(const OBB3D& obb, const Vector4& color) {
	// 各軸 * 半サイズ
	const Vector3 xAxis = obb.orientations[0] * obb.size.x;
	const Vector3 yAxis = obb.orientations[1] * obb.size.y;
	const Vector3 zAxis = obb.orientations[2] * obb.size.z;

	// 8頂点（インデックスのビット0=X / ビット1=Y / ビット2=Z が+側かどうか）
	Vector3 corners[8];
	for (int i = 0; i < 8; ++i) {
		corners[i] = obb.center
			+ ((i & 1) ? xAxis : -xAxis)
			+ ((i & 2) ? yAxis : -yAxis)
			+ ((i & 4) ? zAxis : -zAxis);
	}

	// 12本の辺（1軸だけ異なる頂点同士を結ぶ）
	for (int i = 0; i < 8; ++i) {
		for (int bit = 1; bit < 8; bit <<= 1) {
			if ((i & bit) == 0) {
				DrawLine(corners[i], corners[i | bit], color);
			}
		}
	}
}

void DebugDraw::DrawTriangle(const Triangle3D& triangle, const Vector4& color) {
	DrawLine(triangle.v0, triangle.v1, color);
	DrawLine(triangle.v1, triangle.v2, color);
	DrawLine(triangle.v2, triangle.v0, color);
}

void DebugDraw::DrawCapsule(const Capsule3D& capsule, const Vector4& color) {
	const Vector3& start = capsule.segment.start;
	const Vector3& end = capsule.segment.end;

	// 両端の球
	DrawSphere({ start, capsule.radius }, color);
	DrawSphere({ end, capsule.radius }, color);

	// 側面：軸に垂直な4方向で両端の球をつなぐ（長さ0なら球だけ）
	const Vector3 axis = end - start;
	if (Length(axis) <= 0.0f) {
		return;
	}
	const Vector3 side = Perpendicular(axis);
	const Vector3 side2 = Normalize(Cross(axis, side));
	const Vector3 offsets[4] = { side, -side, side2, -side2 };
	for (const Vector3& offset : offsets) {
		DrawLine(start + offset * capsule.radius, end + offset * capsule.radius, color);
	}
}

void DebugDraw::DrawPlane(const Plane3D& plane, const Vector4& color, float size) {
	// 平面上で原点に最も近い点（dot(normal, p) + distance = 0 より p = -distance * normal）
	const Vector3 center = plane.normal * -plane.distance;

	// 平面に沿った2方向
	const Vector3 u = Perpendicular(plane.normal) * (size * 0.5f);
	const Vector3 v = Normalize(Cross(plane.normal, u)) * (size * 0.5f);

	// 四角の枠
	const Vector3 corners[4] = { center + u + v, center - u + v, center - u - v, center + u - v };
	for (int i = 0; i < 4; ++i) {
		DrawLine(corners[i], corners[(i + 1) % 4], color);
	}

	// 法線（平面の表の向き）
	DrawLine(center, center + plane.normal * (size * 0.5f), color);
}

void DebugDraw::DrawCurve(const Curve& curve, const Vector4& color, bool drawControlPoints) {
	// 曲線を折れ線で近似する
	Vector3 previous = curve.GetPoint(0.0f);
	for (uint32_t i = 1; i <= kCurveSubdivision; ++i) {
		const Vector3 current = curve.GetPoint(float(i) / float(kCurveSubdivision));
		DrawLine(previous, current, color);
		previous = current;
	}

	if (!drawControlPoints) {
		return;
	}

	// 制御点を結ぶ線（薄い色）と、各制御点の十字
	const Vector4 guideColor = { color.x, color.y, color.z, color.w * 0.4f };
	const std::array<Vector3, 3>& points = curve.GetControlPoints();
	DrawLine(points[0], points[1], guideColor);
	DrawLine(points[1], points[2], guideColor);
	for (const Vector3& point : points) {
		DrawLine(point - Vector3{ kControlPointMarkSize, 0.0f, 0.0f }, point + Vector3{ kControlPointMarkSize, 0.0f, 0.0f }, color);
		DrawLine(point - Vector3{ 0.0f, kControlPointMarkSize, 0.0f }, point + Vector3{ 0.0f, kControlPointMarkSize, 0.0f }, color);
		DrawLine(point - Vector3{ 0.0f, 0.0f, kControlPointMarkSize }, point + Vector3{ 0.0f, 0.0f, kControlPointMarkSize }, color);
	}
}

void DebugDraw::DrawGrid(float size, uint32_t divisions, const Vector4& color) {
	if (divisions == 0) {
		return;
	}
	const float half = size * 0.5f;
	const float step = size / float(divisions);
	for (uint32_t i = 0; i <= divisions; ++i) {
		const float offset = -half + step * float(i);
		DrawLine({ offset, 0.0f, -half }, { offset, 0.0f, half }, color);  // Z方向の線
		DrawLine({ -half, 0.0f, offset }, { half, 0.0f, offset }, color);  // X方向の線
	}
}

// ===================== エンジン内部 =====================

DebugDraw* DebugDraw::GetInstance() {
	static DebugDraw instance;
	return &instance;
}

void DebugDraw::Initialize(ID3D12Device* device) {
	// --- ルートシグネチャ（b0[VS]：ビュー射影のみ）---
	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameter.Descriptor.ShaderRegister = 0;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = &rootParameter;
	rootSignatureDesc.NumParameters = 1;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	hr = device->CreateRootSignature(
		0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

	// --- シェーダー ---
	ComPtr<IDxcBlob> vertexShader = ShaderCompiler::GetInstance()->Compile(L"Line.VS.hlsl", L"vs_6_0");
	assert(vertexShader != nullptr);
	ComPtr<IDxcBlob> pixelShader = ShaderCompiler::GetInstance()->Compile(L"Line.PS.hlsl", L"ps_6_0");
	assert(pixelShader != nullptr);

	// --- PSO（ライン・カリングなし・深度テストなし＝常に手前・半透明可）---
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "COLOR";
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_BLEND_DESC blendDesc{};
	D3D12_RENDER_TARGET_BLEND_DESC& renderTarget = blendDesc.RenderTarget[0];
	renderTarget.BlendEnable = TRUE;
	renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	renderTarget.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
	renderTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
	renderTarget.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = false;

	PipelineManager::PipelineConfig config{};
	config.device = device;
	config.rootSignature = rootSignature_.Get();
	config.inputLayout = { inputElementDescs, _countof(inputElementDescs) };
	config.blendDesc = blendDesc;
	config.rasterizerDesc = rasterizerDesc;
	config.depthStencilDesc = depthStencilDesc;
	config.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	config.vertexShader = vertexShader;
	config.pixelShader = pixelShader;
	pipelineState_ = PipelineManager::CreateGraphicsPipeline(config);

	// --- 頂点バッファ（フレームインフライト数ぶん）---
	vertexResource_ = CreateBufferResource(
		device, sizeof(LineVertex) * size_t(kMaxVertexCount) * DirectXCore::kFramesInFlight);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices_));

	vertices_.reserve(1024);
}

void DebugDraw::Finalize() {
	vertices_.clear();
	vertexResource_.Reset();
	mappedVertices_ = nullptr;
	pipelineState_.Reset();
	rootSignature_.Reset();
}

void DebugDraw::AddLine(const Vector3& start, const Vector3& end, const Vector4& color) {
	if (vertices_.size() + 2 > kMaxVertexCount) {
		if (!overflowLogged_) {
			Log("DebugDraw: too many lines in one frame. Extra lines are not drawn.\n");
			overflowLogged_ = true;
		}
		return;
	}
	vertices_.push_back({ start, color });
	vertices_.push_back({ end, color });
}

void DebugDraw::Render(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS viewProjection) {
	if (vertices_.empty() || mappedVertices_ == nullptr) {
		return;
	}

	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	const size_t slotOffset = size_t(frameIndex) * kMaxVertexCount;

	// このフレームの最初の描画でGPUへ転送する（立体視で複数回呼ばれても転送は1回）
	if (!uploaded_) {
		uploadedVertexCount_ = uint32_t(vertices_.size());
		std::memcpy(mappedVertices_ + slotOffset, vertices_.data(), sizeof(LineVertex) * uploadedVertexCount_);
		uploaded_ = true;
	}

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation =
		vertexResource_->GetGPUVirtualAddress() + UINT64(slotOffset) * sizeof(LineVertex);
	vertexBufferView.SizeInBytes = UINT(sizeof(LineVertex) * uploadedVertexCount_);
	vertexBufferView.StrideInBytes = sizeof(LineVertex);

	// 線描画専用のルートシグネチャ・PSOで描く（この後にシーンの描画は続かない）
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->SetGraphicsRootConstantBufferView(0, viewProjection);
	commandList->DrawInstanced(uploadedVertexCount_, 1, 0, 0);
}

void DebugDraw::EndFrame() {
	vertices_.clear();
	uploaded_ = false;
	uploadedVertexCount_ = 0;
}

} // namespace Engine
