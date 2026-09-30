#include "Game/Scene/TitleScene.h"

#include "Engine/Input/Input.h"
#include "Game/Scene/GameScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void TitleScene::OnInitialize() {
	camera_.GetTransform().translate = { 0.0f, 0.0f, -6.0f };

	// 立方体の面の向きが分かるよう、光を斜め上から当てる
	directionalLight_.direction = { 0.4f, -1.0f, 0.6f };

	logo_.Initialize(Primitive::kCube);
	logo_.SetColor({ 1.0f, 0.8f, 0.2f, 1.0f });
}

void TitleScene::OnUpdate() {
	// Enter / Space / パッドのAボタンでゲームへ（切り替わるのは次のフレームの頭）
	Input* input = Input::GetInstance();
	if (input->IsTrigger(DIK_RETURN) || input->IsTrigger(DIK_SPACE) || input->IsPadTrigger(kPadA)) {
		ChangeScene<GameScene>();
	}

	// 仮ロゴをゆっくり回す
	logo_.GetTransform().rotate.x += 0.01f;
	logo_.GetTransform().rotate.y += 0.02f;
	logo_.Update();
}

void TitleScene::OnDraw() {
	logo_.Draw();
}

#ifdef USE_IMGUI
void TitleScene::OnDrawImGui() {
	ImGui::Begin("Title");
	ImGui::Text("Press Enter / Space / A to start");
	ImGui::End();
}
#endif
