#include "Game/Scene/ClearScene.h"

#include "Engine/Input/Input.h"
#include "Game/Scene/TitleScene.h"
#include "Game/Sound/SoundEffects.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void ClearScene::OnInitialize() {
	camera_.GetTransform().translate = { 0.0f, 0.0f, -6.0f };

	// 球の陰影が分かるよう、光を斜め上から当てる
	directionalLight_.direction = { 0.4f, -1.0f, 0.6f };

	skydome_.Initialize();
	// 文字。GameClear だけ上下にゆっくり揺らす（press space はタイトルと調整値を共通にする）
	clearObj_.Initialize("resources/textObjects/gameClear.obj", "ClearText", { 0.0f, 0.5f, 0.0f }, 0.7f, 0.15f);
	pressSpace_.Initialize("resources/textObjects/pressSpace.obj", "PressSpace", { 0.0f, -0.6f, 0.0f }, 0.4f, 0.0f);

	// フェードインから始める
	fade_.Initialize();
	fade_.Start(Fade::Status::kFadeIn, Fade::GetDefaultDuration());
	phase_ = Phase::kFadeIn;

	// クリアのジングル
	SoundEffects::Play(SoundEffects::Id::kClear);
}

void ClearScene::OnUpdate() {

	// 天球（背景）はどの段階でも描くので、毎フレーム更新する
	skydome_.Update();

	clearObj_.Update();
	pressSpace_.Update();

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
			SoundEffects::Play(SoundEffects::Id::kDecide);
			fade_.Start(Fade::Status::kFadeOut, Fade::GetDefaultDuration());
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
	// 不透明なものを先に描く（背景の天球が一番奥）
	skydome_.Draw();
	clearObj_.Draw();
	pressSpace_.Draw();

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
