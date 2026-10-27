#include "Game/MyGame.h"

#include "Game/Scene/BossScene.h"
#include "Game/Scene/TitleScene.h"
#include "Game/Sound/SoundEffects.h"
#include "Game/Util/GlobalVariables.h"

void MyGame::Initialize() {
	// ウィンドウのタイトル（エンジンの初期化より前に設定すると、最初からこのタイトルで開く）
	SetWindowTitle(L"AL4 Game");

	// エンジン各サブシステムの初期化
	Framework::Initialize();

	// 効果音をまとめて読み込む（エンジンの Audio の初期化の後）
	SoundEffects::Load();

	// 調整値: 保存済みの CSV を読んでから各クラスの既定値を登録し（読み込み済みの項目は上書きされない）、
	// 最初のシーンを作る前に反映しておく
	GlobalVariables::GetInstance()->LoadFiles();
	BossScene::RegisterGlobalVariables();
	BossScene::ApplyGlobalVariables();

	// 最初のシーン
	ChangeScene<TitleScene>();
}

#ifdef USE_IMGUI
void MyGame::DrawImGui() {
	// シーンの ImGui のあとに、調整値のウィンドウを全シーン共通で出す
	Framework::DrawImGui();
	GlobalVariables::GetInstance()->DrawImGui();
}
#endif
