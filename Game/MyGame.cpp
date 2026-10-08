#include "Game/MyGame.h"

#include "Game/Scene/TitleScene.h"

void MyGame::Initialize() {
	// ウィンドウのタイトル（エンジンの初期化より前に設定すると、最初からこのタイトルで開く）
	SetWindowTitle(L"AL4 Game");

	// エンジン各サブシステムの初期化
	Framework::Initialize();

	// 最初のシーン
	ChangeScene<TitleScene>();
}
