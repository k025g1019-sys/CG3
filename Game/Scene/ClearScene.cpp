#include "Game/Scene/ClearScene.h"

#include "Engine/Input/Input.h"
#include "Game/Scene/TitleScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void ClearScene::OnInitialize() {
	camera_.GetTransform().translate = { 0.0f, 0.0f, -6.0f };

	// 球の陰影が分かるよう、光を斜め上から当てる
	directionalLight_.direction = { 0.4f, -1.0f, 0.6f };

	mark_.Initialize(Primitive::kSphere);
	mark_.SetColor({ 0.3f, 0.9f, 0.4f, 1.0f });

	// フェードインから始める
	fade_.Initialize();
	fade_.Start(Fade::Status::kFadeIn, kFadeDuration);
	phase_ = Phase::kFadeIn;
}

void ClearScene::OnUpdate() {
	// 仮モデルをゆっくり回す
	mark_.GetTransform().rotate.y += 0.02f;
	mark_.Update();

	fade_.Update();

	Input* input = Input::GetInstance();
	switch (phase_) {
	case Phase::kFadeIn:
		// フェードインが終わったら入力待ちへ（最前面のスプライトは止めておく）
		if (fade_.IsFinished()) {
			fade_.Stop();
			phase_ = Phase::kMain;
		}
		break;
	case Phase::kMain:
		// Enter / Space でフェードアウトを始める
		if (input->IsTrigger(DIK_RETURN) || input->IsTrigger(DIK_SPACE)) {
			fade_.Start(Fade::Status::kFadeOut, kFadeDuration);
			phase_ = Phase::kFadeOut;
		}
		break;
	case Phase::kFadeOut:
		// フェードアウトが終わったらタイトルへ（切り替わるのは次のフレームの頭）
		if (fade_.IsFinished()) {
			ChangeScene<TitleScene>();
		}
		break;
	}
}

void ClearScene::OnDraw() {
	mark_.Draw();

	// フェードは一番最後（最前面）
	fade_.Draw();
}

#ifdef USE_IMGUI
void ClearScene::OnDrawImGui() {
	ImGui::Begin("Clear");
	ImGui::Text("CLEAR!  Press Enter / Space to return to the title");
	ImGui::End();
}
#endif
