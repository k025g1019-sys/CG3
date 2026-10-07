#ifdef USE_IMGUI

#include "Engine/Core/ImGuiManager.h"

#include <cassert>
#include <d3dcompiler.h>

#include "Engine/Core/DirectXCore.h"
#include "Engine/Core/WinApp.h"
#include "Engine/Graphics/DescriptorHeapManager.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"  // ドッキングのDockBuilder API
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

// 点サンプリング用PSOのシェーダーコンパイル（ImGuiのDX12バックエンドと同じくD3DCompileを使う）
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace Engine {

namespace {

// ImGuiのDX12バックエンド（imgui_impl_dx12.cpp）と同じ頂点／ピクセルシェーダー。
// 点サンプリング用PSOでも描画結果（位置・頂点色×テクスチャ）を同じにするため、同じソースをコンパイルする
const char kImGuiVertexShader[] =
	"cbuffer vertexBuffer : register(b0)"
	"{"
	"  float4x4 ProjectionMatrix;"
	"};"
	"struct VS_INPUT"
	"{"
	"  float2 pos : POSITION;"
	"  float4 col : COLOR0;"
	"  float2 uv  : TEXCOORD0;"
	"};"
	"struct PS_INPUT"
	"{"
	"  float4 pos : SV_POSITION;"
	"  float4 col : COLOR0;"
	"  float2 uv  : TEXCOORD0;"
	"};"
	"PS_INPUT main(VS_INPUT input)"
	"{"
	"  PS_INPUT output;"
	"  output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));"
	"  output.col = input.col;"
	"  output.uv  = input.uv;"
	"  return output;"
	"}";

const char kImGuiPixelShader[] =
	"struct PS_INPUT"
	"{"
	"  float4 pos : SV_POSITION;"
	"  float4 col : COLOR0;"
	"  float2 uv  : TEXCOORD0;"
	"};"
	"SamplerState sampler0 : register(s0);"
	"Texture2D texture0 : register(t0);"
	"float4 main(PS_INPUT input) : SV_Target"
	"{"
	"  return input.col * texture0.Sample(sampler0, input.uv);"
	"}";

} // namespace

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(WinApp::GetInstance()->GetHwnd());

	// フォント用にSRVヒープのスロットを1つ確保して使う
	DescriptorHeapManager* heapManager = DescriptorHeapManager::GetInstance();
	uint32_t srvIndex = heapManager->AllocateSrv();
	ImGui_ImplDX12_Init(
		DirectXCore::GetInstance()->GetDevice(),
		DirectXCore::GetSwapChainBufferCount(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DirectXCore::GetInstance()->GetSRVDescriptorHeap(),
		heapManager->GetSrvCPUHandle(srvIndex),
		heapManager->GetSrvGPUHandle(srvIndex));

	ImGui::GetIO().Fonts->Build();

	// 画像を拡大してもにじまない描画用（PushPointSampling）のパイプライン
	CreatePointSamplingPipeline();
}

void ImGuiManager::CreatePointSamplingPipeline() {
	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

	// --- ルートシグネチャ ---
	// ImGuiのDX12バックエンドと同じ並び（0:b0の行列16個のルート定数 / 1:t0のSRVテーブル / s0の静的サンプラ）にする。
	// 並びが同じなので、バックエンドが描画ごとに行うSRVテーブルの設定（パラメータ1）がこのPSOでもそのまま効く。
	// 違いはサンプラだけで、バイリニアではなく最近傍（POINT）にする
	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.NumDescriptors = 1;
	descriptorRange.BaseShaderRegister = 0;
	descriptorRange.RegisterSpace = 0;
	descriptorRange.OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER rootParameters[2]{};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[0].Constants.ShaderRegister = 0;
	rootParameters[0].Constants.RegisterSpace = 0;
	rootParameters[0].Constants.Num32BitValues = 16;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRange;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;  // 最近傍
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0.0f;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = 0.0f;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &sampler;
	rootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	[[maybe_unused]] HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));
	hr = device->CreateRootSignature(
		0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&pointSamplingRootSignature_));
	assert(SUCCEEDED(hr));

	// --- シェーダー（バックエンドと同じソース）---
	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
	hr = D3DCompile(
		kImGuiVertexShader, sizeof(kImGuiVertexShader) - 1, nullptr, nullptr, nullptr,
		"main", "vs_5_0", 0, 0, &vertexShaderBlob, nullptr);
	assert(SUCCEEDED(hr));
	hr = D3DCompile(
		kImGuiPixelShader, sizeof(kImGuiPixelShader) - 1, nullptr, nullptr, nullptr,
		"main", "ps_5_0", 0, 0, &pixelShaderBlob, nullptr);
	assert(SUCCEEDED(hr));

	// --- PSO（バックエンドと同じ: ImDrawVertの入力レイアウト・アルファブレンド・カリングなし・深度なし）---
	const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, UINT(IM_OFFSETOF(ImDrawVert, pos)),
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, UINT(IM_OFFSETOF(ImDrawVert, uv)),
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, UINT(IM_OFFSETOF(ImDrawVert, col)),
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.NodeMask = 1;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.pRootSignature = pointSamplingRootSignature_.Get();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;  // ImGui_ImplDX12_Initに渡したものと同じ
	psoDesc.SampleDesc.Count = 1;
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };

	D3D12_RENDER_TARGET_BLEND_DESC& blend = psoDesc.BlendState.RenderTarget[0];
	blend.BlendEnable = TRUE;
	blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blend.BlendOp = D3D12_BLEND_OP_ADD;
	blend.SrcBlendAlpha = D3D12_BLEND_ONE;
	blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;

	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pointSamplingPipelineState_));
	assert(SUCCEEDED(hr));
}

void ImGuiManager::PushPointSampling(ImDrawList* drawList) {
	// ここから先の描画コマンドの前に、点サンプリング用PSOへ切り替えるコールバックを挟む
	drawList->AddCallback(PointSamplingCallback, this);
}

void ImGuiManager::PopPointSampling(ImDrawList* drawList) {
	// バックエンド標準の描画状態（PSO・ルートシグネチャ・ルート定数）へ戻す
	drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

void ImGuiManager::PointSamplingCallback(const ImDrawList* parentList, const ImDrawCmd* cmd) {
	(void)parentList;
	ImGuiManager* self = static_cast<ImGuiManager*>(cmd->UserCallbackData);
	ID3D12GraphicsCommandList* commandList = self->currentCommandList_;
	if (commandList == nullptr || !self->pointSamplingPipelineState_) {
		return;
	}

	// ルートシグネチャを差し替えるとルート引数が無効になるため、バックエンドと同じ正射影
	// （ImGui座標 → クリップ空間）をルート定数へ書き直す。SRVテーブルは続く描画でバックエンドが設定する
	const ImDrawData* drawData = ImGui::GetDrawData();
	const float left = drawData->DisplayPos.x;
	const float right = drawData->DisplayPos.x + drawData->DisplaySize.x;
	const float top = drawData->DisplayPos.y;
	const float bottom = drawData->DisplayPos.y + drawData->DisplaySize.y;
	const float mvp[4][4] = {
		{ 2.0f / (right - left),           0.0f,                            0.0f, 0.0f },
		{ 0.0f,                            2.0f / (top - bottom),           0.0f, 0.0f },
		{ 0.0f,                            0.0f,                            0.5f, 0.0f },
		{ (right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f },
	};

	commandList->SetGraphicsRootSignature(self->pointSamplingRootSignature_.Get());
	commandList->SetPipelineState(self->pointSamplingPipelineState_.Get());
	commandList->SetGraphicsRoot32BitConstants(0, 16, mvp, 0);
}

void ImGuiManager::SetDefaultDockLayout(
	std::vector<std::string> topRight, std::vector<std::string> bottomRight) {
	defaultDockTopRight_ = std::move(topRight);
	defaultDockBottomRight_ = std::move(bottomRight);
}

void ImGuiManager::BeginFrame() {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// --- 画面全体を覆うドックスペース ---
	// 非表示のホストウィンドウを画面いっぱいに固定し、その中をドッキング先にする。
	// 中央ノードは透過（Passthru）で、ゲーム画面の表示とマウス入力を妨げない。
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	const ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("##DockHost", nullptr, hostFlags);
	ImGui::PopStyleVar(3);

	// ドックスペースのID（ウィンドウ名から決まり、起動をまたいで同一）。
	// ノードが無い＝imgui.iniに保存済みレイアウトが無い初回起動なので、デフォルト配置を組む
	const ImGuiID dockspaceId = ImGui::GetID("EngineDockSpace");
	if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
		BuildDefaultDockLayout(dockspaceId, viewport->WorkSize.x, viewport->WorkSize.y);
	}
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	// 中央ノード（どのウィンドウにも占有されていない領域）＝ゲームの表示先を記録する
	if (const ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockspaceId)) {
		gameArea_ = { central->Pos.x, central->Pos.y, central->Size.x, central->Size.y };
	}
}

void ImGuiManager::BuildDefaultDockLayout(unsigned int dockspaceId, float width, float height) {
	// 左＝ゲーム（中央ノード）／右上＝デバッグUI（タブ結合）／右下＝Webカメラ の初期配置。
	// 以降のレイアウト変更はimgui.iniへ保存されるため、ここは初回起動時のみ通る
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, ImVec2(width, height));

	ImGuiID centralId = dockspaceId;
	ImGuiID rightTopId =
		ImGui::DockBuilderSplitNode(centralId, ImGuiDir_Right, 0.5f, nullptr, &centralId);
	ImGuiID rightBottomId =
		ImGui::DockBuilderSplitNode(rightTopId, ImGuiDir_Down, 0.5f, nullptr, &rightTopId);

	for (const std::string& name : defaultDockTopRight_) {
		ImGui::DockBuilderDockWindow(name.c_str(), rightTopId);
	}
	for (const std::string& name : defaultDockBottomRight_) {
		ImGui::DockBuilderDockWindow(name.c_str(), rightBottomId);
	}
	ImGui::DockBuilderFinish(dockspaceId);
}

void ImGuiManager::Render() {
	ImGui::Render();
}

void ImGuiManager::Draw(ID3D12GraphicsCommandList* commandList) {
	// 描画中だけコマンドリストを控えておく（PointSamplingCallbackが使う）
	currentCommandList_ = commandList;
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	currentCommandList_ = nullptr;
}

void ImGuiManager::Finalize() {
	pointSamplingPipelineState_.Reset();
	pointSamplingRootSignature_.Reset();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

} // namespace Engine

#endif  // USE_IMGUI
