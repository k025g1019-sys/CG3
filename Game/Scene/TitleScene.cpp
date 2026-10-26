#include "Game/Scene/TitleScene.h"

#include "Engine/Input/Input.h"
#include "Game/Scene/BossScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void TitleScene::OnInitialize() {
	camera_.GetTransform().translate = { 0.0f, 0.0f, -6.0f };

	// 文字の厚みの陰影が分かるよう、光を斜め上から当てる
	directionalLight_.direction = { 0.4f, -1.0f, 0.6f };

	skydome_.Initialize();

	// タイトルロゴ（上下にゆっくり揺らす。大きさなどは ImGui の TitleLogo で調整する）
	logo_.Initialize("resources/textObjects/title.obj", "TitleLogo", { 0.0f, 0.3f, 0.0f }, 0.8f, 0.1f);

	// 「press space」の表示（調整値はクリア画面と共通）
	pressSpace_.Initialize("resources/textObjects/pressSpace.obj", "PressSpace", { 0.0f, -0.6f, 0.0f }, 0.4f, 0.0f);

	// 「マウス推奨」の文字（ロゴの右下に揺らさず置く）
	caution_.Initialize("resources/textObjects/caution.obj", "Caution", { 1.63f, -0.15f, 0.0f }, 0.15f, 0.0f);

	// フェードインから始める
	fade_.Initialize();
	fade_.Start(Fade::Status::kFadeIn, Fade::GetDefaultDuration());
	phase_ = Phase::kFadeIn;
}

void TitleScene::OnUpdate() {

	// 天球（背景）はどの段階でも描くので、毎フレーム更新する
	skydome_.Update();

	logo_.Update();

	pressSpace_.Update();

	caution_.Update();

	// フェードの時間の調整値を反映（ImGui での変更をこのシーンでも反映する）
	Fade::ApplyGlobalVariables();
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
			fade_.Start(Fade::Status::kFadeOut, Fade::GetDefaultDuration());
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
	// 不透明なものを先に描く（背景の天球が一番奥）
	skydome_.Draw();
	logo_.Draw();
	pressSpace_.Draw();
	caution_.Draw();

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
