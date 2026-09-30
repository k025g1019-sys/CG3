#include "Game/Scene/GameScene.h"

#include "Engine/Input/Input.h"
#include "Game/Scene/TitleScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void GameScene::OnInitialize() {
	// カメラ（少し上から見下ろす）
	camera_.GetTransform().translate = { 0.0f, 3.0f, -12.0f };
	camera_.GetTransform().rotate = { 0.15f, 0.0f, 0.0f };

	// 立体の面の向きが分かるよう、光を斜め上から当てる
	directionalLight_.direction = { 0.4f, -1.0f, 0.6f };

	// 地面（仮モデル：平面を広げて使う。上面の高さは0）
	ground_.Initialize(Primitive::kPlane);
	ground_.GetTransform().scale = { 20.0f, 1.0f, 20.0f };
	ground_.SetColor({ 0.35f, 0.7f, 0.35f, 1.0f });

	player_.Initialize();
}

void GameScene::OnUpdate() {
	// Escでタイトルへ戻る（切り替わるのは次のフレームの頭）
	if (Input::GetInstance()->IsTrigger(DIK_ESCAPE)) {
		ChangeScene<TitleScene>();
	}

	player_.Update();
	ground_.Update();
}

void GameScene::OnDraw() {
	ground_.Draw();
	player_.Draw();
}

#ifdef USE_IMGUI
void GameScene::OnDrawImGui() {
	ImGui::Begin("Game");

	Transform3D& cameraTransform = camera_.GetTransform();
	ImGui::DragFloat3("Camera rotate", &cameraTransform.rotate.x, 0.01f);
	ImGui::DragFloat3("Camera translate", &cameraTransform.translate.x, 0.01f);
	ImGui::Separator();

	player_.DrawImGui();

	ImGui::End();
}
#endif
