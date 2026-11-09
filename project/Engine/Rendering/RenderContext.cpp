#include "RenderContext.h"

#include <cassert>

#include "../Core/DirectXCore.h"
#include "../Core/WinApp.h"
#include "../Graphics/PipelineManager.h"

namespace Engine {

RenderContext* RenderContext::GetInstance() {
	static RenderContext instance;
	return &instance;
}

RenderContext::RenderContext()
	: screenWidth_(float(WinApp::kClientWidth))
	, screenHeight_(float(WinApp::kClientHeight)) {
}

ID3D12GraphicsCommandList* RenderContext::GetCommandList() const {
	return DirectXCore::GetInstance()->GetCommandList();
}

void RenderContext::BindStandardState(ID3D12GraphicsCommandList* commandList) const {
	// BaseScene::Drawの中（ビュー射影・光源のCBVが設定された後）でだけ呼べる
	assert(viewProjectionAddress_ != 0);
	assert(directionalLightAddress_ != 0 && pointLightsAddress_ != 0);

	PipelineManager* pipelineManager = PipelineManager::GetInstance();
	commandList->SetGraphicsRootSignature(pipelineManager->GetRootSignature());
	commandList->SetGraphicsRootConstantBufferView(PipelineManager::kRootViewProjection, viewProjectionAddress_);
	commandList->SetGraphicsRootConstantBufferView(PipelineManager::kRootDirectionalLight, directionalLightAddress_);
	commandList->SetGraphicsRootConstantBufferView(PipelineManager::kRootPointLights, pointLightsAddress_);

	// 標準PSO（裏面カリング・三角形）
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kStandard);
}

} // namespace Engine
