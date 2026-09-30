#include "Engine/Scene/BaseScene.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Core/WinApp.h"
#include "Engine/Culling/FrustumCulling.h"
#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Rendering/RenderContext.h"

namespace Engine {

void BaseScene::Initialize() {
	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

	// --- 光源（シーン共通。毎フレームUpdateで書き込む）---
	directionalLightCB_.Create(device, DirectXCore::kFramesInFlight);
	pointLightCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- 立体視：視点ごとのビュー射影CBuffer ---
	stereoCamera_.Initialize(device);

	// --- 派生シーンのオブジェクト生成 ---
	OnInitialize();
}

void BaseScene::Update() {
	RenderContext* context = RenderContext::GetInstance();

	// スプライトの正射影・画面外判定に使う描画先サイズ
	context->SetScreenSize(GetRenderAreaWidth(), GetRenderAreaHeight());

	// --- 派生シーンのゲーム処理とオブジェクト更新 ---
	OnUpdate();

	// --- カメラ（OnUpdateで動かした結果を、同じフレームの描画にそのまま反映する）---
	// 投影のアスペクト比は描画先矩形に合わせる（リサイズやドッキングの変更で物体が伸び縮みしないように）
	projection_ = camera_.GetProjectionMatrix(GetAspectRatio());
	view_ = CalcViewMatrix();

	// 視錐台カリングは中心カメラの視錐台で判定する（視点間のずれは眼間距離程度で無視できる）
	context->SetFrustum(MakeFrustumFromViewProjection(view_ * projection_));

	// --- 光源 ---
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	directionalLightCB_.Write(frameIndex, directionalLight_);
	pointLightCB_.Write(frameIndex, pointLights_);

	// --- 立体視：中心カメラから各視点（眼）のビュー射影を更新する ---
	stereoCamera_.Update(view_, projection_);
}

void BaseScene::Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) {
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	PipelineManager* pipelineManager = PipelineManager::GetInstance();

	// この視点のビュー射影（スプライト等が一時的に差し替えた後に戻せるよう共有しておく）
	const D3D12_GPU_VIRTUAL_ADDRESS viewProjection =
		stereoCamera_.GetViewProjectionAddress(frameIndex, viewIndex);
	RenderContext::GetInstance()->SetViewProjectionAddress(viewProjection);

	// --- 共通設定（Viewport/Scissor/RenderTarget/DescriptorHeapは
	//     DirectXCore::BeginFrameまたはStereoRenderer::BeginViewで設定済み）---
	commandList->SetGraphicsRootSignature(pipelineManager->GetRootSignature());
	commandList->SetGraphicsRootConstantBufferView(PipelineManager::kRootViewProjection, viewProjection);
	commandList->SetGraphicsRootConstantBufferView(
		PipelineManager::kRootDirectionalLight, directionalLightCB_.GetGPUAddress(frameIndex));
	commandList->SetGraphicsRootConstantBufferView(
		PipelineManager::kRootPointLights, pointLightCB_.GetGPUAddress(frameIndex));

	// 標準PSO（裏面カリング・三角形）で描き始める
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kStandard);

	// --- 派生シーンのオブジェクト描画 ---
	OnDraw();
}

float BaseScene::GetRenderAreaX() const {
	return (renderAreaWidth_ > 0.0f && renderAreaHeight_ > 0.0f) ? renderAreaX_ : 0.0f;
}

float BaseScene::GetRenderAreaY() const {
	return (renderAreaWidth_ > 0.0f && renderAreaHeight_ > 0.0f) ? renderAreaY_ : 0.0f;
}

float BaseScene::GetRenderAreaWidth() const {
	return (renderAreaWidth_ > 0.0f && renderAreaHeight_ > 0.0f)
		? renderAreaWidth_
		: float(WinApp::GetInstance()->GetClientWidth());
}

float BaseScene::GetRenderAreaHeight() const {
	return (renderAreaWidth_ > 0.0f && renderAreaHeight_ > 0.0f)
		? renderAreaHeight_
		: float(WinApp::GetInstance()->GetClientHeight());
}

} // namespace Engine
