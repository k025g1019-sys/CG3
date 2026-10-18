#pragma once

#include "Engine/Rendering/Object3D.h"
#include "Engine/Scene/BaseScene.h"
#include "Game/UI/Fade.h"

/// <summary>
/// タイトル画面（仮）。フェードインのあと Enter / Space でフェードアウトし、ボス戦へ進む。
/// </summary>
class TitleScene : public Engine::BaseScene {
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
		kFadeOut,  // フェードアウト（終わったらボス戦へ）
	};

	Engine::Object3D logo_;         // タイトルロゴの代わりの仮モデル（回転する立方体）
	Fade fade_;                     // 画面の出入りのフェード
	Phase phase_ = Phase::kFadeIn;  // 現在の段階
};
