#include "Game/Scene/TitleScene.h"

#include "Engine/Input/Input.h"
#include "Game/Scene/BossScene.h"

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

	// フェードインから始める
	fade_.Initialize();
	fade_.Start(Fade::Status::kFadeIn, kFadeDuration);
	phase_ = Phase::kFadeIn;
}

void TitleScene::OnUpdate() {
	// 仮ロゴをゆっくり回す
	logo_.GetTransform().rotate.x += 0.01f;
	logo_.GetTransform().rotate.y += 0.02f;
	logo_.Update();

	fade_.Update(GetRenderAreaWidth(), GetRenderAreaHeight());

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
		// フェードアウトが終わったらボス戦へ（切り替わるのは次のフレームの頭）
		if (fade_.IsFinished()) {
			ChangeScene<BossScene>();
		}
		break;
	}
}

void TitleScene::OnDraw() {
	logo_.Draw();

	// フェードは一番最後（最前面）
	fade_.Draw();
}

#ifdef USE_IMGUI
void TitleScene::OnDrawImGui() {
	ImGui::Begin("Title");
	ImGui::Text("Press Enter / Space to start");
	ImGui::End();
}
#endif
