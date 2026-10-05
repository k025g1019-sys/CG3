#include "Engine/Scene/SceneManager.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Scene/BaseScene.h"

namespace Engine {

SceneManager* SceneManager::GetInstance() {
	static SceneManager instance;
	return &instance;
}

SceneManager::~SceneManager() = default;

void SceneManager::ChangeScene(std::unique_ptr<BaseScene> nextScene) {
	nextScene_ = std::move(nextScene);
}

void SceneManager::ApplySceneChange() {
	if (!nextScene_) {
		return;
	}

	// 実行中のフレームが前のシーンの定数バッファ等を参照し終えるのを待ってから破棄する
	if (currentScene_) {
		DirectXCore::GetInstance()->WaitForGPU();
		currentScene_.reset();
	}

	// 次のシーンを初期化する（テクスチャの転送コマンドは記録中のコマンドリストに積まれ、
	// このフレームの描画より先に実行される）
	currentScene_ = std::move(nextScene_);
	currentScene_->SetRenderArea(renderAreaX_, renderAreaY_, renderAreaWidth_, renderAreaHeight_);
	currentScene_->Initialize();
}

void SceneManager::SetRenderArea(float x, float y, float width, float height) {
	renderAreaX_ = x;
	renderAreaY_ = y;
	renderAreaWidth_ = width;
	renderAreaHeight_ = height;

	if (currentScene_) {
		currentScene_->SetRenderArea(x, y, width, height);
	}
}

void SceneManager::Update() {
	if (currentScene_) {
		currentScene_->Update();
	}
}

void SceneManager::Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) {
	if (currentScene_) {
		currentScene_->Draw(commandList, viewIndex);
	}
}

#ifdef USE_IMGUI
void SceneManager::DrawImGui() {
	if (currentScene_) {
		currentScene_->DrawImGui();
	}
}
#endif

void SceneManager::Finalize() {
	nextScene_.reset();
	currentScene_.reset();
}

} // namespace Engine
