#include "Game/Object/AxisGizmo.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Math/Matrix4x4.h"
#include "Engine/Rendering/VertexData.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

// 軸線の長さ＝対象のワールドバウンディング球半径×この係数
// （線の始点は回転中心translateで、バウンディング球の中心とは限らないため、
//   メッシュ中心が原点からずれたモデルでは球面と一致しない。見やすいよう大きめにする）
constexpr float kLengthScale = 1.5f;

}  // namespace

void AxisGizmo::Initialize(ID3D12Device* device) {
	// --- 軸線メッシュとマテリアル（原点から各軸の+方向へ長さ1の線分。X=赤/Y=緑/Z=青）---
	const Vector3 kDirections[kAxisCount] = {
		{ 1.0f, 0.0f, 0.0f },  // X
		{ 0.0f, 1.0f, 0.0f },  // Y
		{ 0.0f, 0.0f, 1.0f },  // Z
	};
	const Vector4 kColors[kAxisCount] = {
		{ 1.0f, 0.0f, 0.0f, 1.0f },  // X=赤
		{ 0.0f, 1.0f, 0.0f, 1.0f },  // Y=緑
		{ 0.0f, 0.0f, 1.0f, 1.0f },  // Z=青
	};

	for (int i = 0; i < kAxisCount; ++i) {
		// texcoord/normalはラインPSO（ライティング無効・白テクスチャ）では使われないがダミー値を入れておく
		VertexData vertices[2]{};
		vertices[0].position = { 0.0f, 0.0f, 0.0f, 1.0f };
		vertices[1].position = { kDirections[i].x, kDirections[i].y, kDirections[i].z, 1.0f };
		for (VertexData& vertex : vertices) {
			vertex.texcoord = { 0.0f, 0.0f };
			vertex.normal = { 0.0f, 1.0f, 0.0f };
		}
		axisMeshes_[i].Create(device, vertices, 2);

		materials_[i].color = kColors[i];
		materials_[i].lightingMode = LightingMode::kNone;
		materials_[i].uvTransform = MakeIdentity4x4();
		materialCBs_[i].Create(device, DirectXCore::kFramesInFlight);
	}

	// --- Transform（3本の軸線で共有）---
	transformCB_.Create(device, DirectXCore::kFramesInFlight);

	// 共通ルートシグネチャはテクスチャSRVを要求するため、1x1白を貼る（マテリアル色がそのまま出る）
	whiteTextureHandle_ = TextureManager::GetInstance()->GetWhiteTexture();
}

void AxisGizmo::Update(const Object3D* target) {
	hasTarget_ = show_ && (target != nullptr);
	if (!hasTarget_) {
		return;
	}

	// 長さは対象の大きさに連動させる（小さいモデルでも大きいモデルでも見やすい比率になる）
	const float length = target->CalcWorldBoundingSphere().radius * kLengthScale;

	// 対象の回転・位置だけを反映する（スケールは長さで置き換え。原点＝回転の中心translateに置く）
	const Transform3D& transform = target->GetTransform();
	Matrix4x4 world = MakeAffineMatrix({ length, length, length }, transform.rotate, transform.translate);

	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	transformCB_.Write(frameIndex, TransformationMatrix{ world });
	for (int i = 0; i < kAxisCount; ++i) {
		materialCBs_[i].Write(frameIndex, materials_[i]);
	}
}

void AxisGizmo::Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS viewProjectionAddress) {
	if (!hasTarget_) {
		return;
	}

	// 深度無効のラインPSOへ切り替える（RootSignature・DescriptorHeap・光源CBVは設定済みの前提）
	commandList->SetPipelineState(PipelineManager::GetInstance()->Get(PipelineManager::Pipeline::kLine));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	// スプライト描画が正射影へ差し替えている場合があるため、この視点のビュー射影を再バインドする
	commandList->SetGraphicsRootConstantBufferView(4, viewProjectionAddress);

	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	commandList->SetGraphicsRootConstantBufferView(1, transformCB_.GetGPUAddress(frameIndex));
	commandList->SetGraphicsRootDescriptorTable(
		3, TextureManager::GetInstance()->GetSrvHandleGPU(whiteTextureHandle_));

	// 軸ごとにマテリアル（色）を差し替えて1本ずつ描く
	for (int i = 0; i < kAxisCount; ++i) {
		commandList->SetGraphicsRootConstantBufferView(0, materialCBs_[i].GetGPUAddress(frameIndex));
		axisMeshes_[i].Draw(commandList);
	}
}

#ifdef USE_IMGUI
void AxisGizmo::DrawImGui() {
	ImGui::Checkbox("Axis Gizmo (selected object)", &show_);
}
#endif
