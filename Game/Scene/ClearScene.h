#pragma once

#include "Engine/Rendering/Object3D.h"
#include "Engine/Scene/BaseScene.h"
#include "Game/Stage/Skydome.h"
#include "Game/Object/TextObjects.h"
#include "Game/UI/Fade.h"

/// <summary>
/// クリア画面（仮）。フェードインのあと Enter / Space でフェードアウトし、タイトルへ戻る。
/// </summary>
class ClearScene : public Engine::BaseScene {
protected:

	void OnInitialize() override;

	void OnUpdate() override;

	void OnDraw() override;

#ifdef USE_IMGUI
	void OnDrawImGui() override;
#endif

private:

	/// <summary>
	/// 画面の段階
	/// </summary>
	enum class Phase {
		kFadeIn,   // フェードイン
		kMain,     // 入力待ち
		kFadeOut,  // フェードアウト（終わったらタイトルへ）
	};

	Skydome skydome_;               // 天球（背景の空）
	TextObjects clearObj_;          // クリア表示（上下にゆっくり揺れる）
	TextObjects pressSpace_;        // 「press space」の表示
	Fade fade_;                     // 画面の出入りのフェード
	Phase phase_ = Phase::kFadeIn;  // 現在の段階
};
