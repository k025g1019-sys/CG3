#pragma once

#include "Engine/Core/Framework.h"

/// <summary>
/// ゲームのアプリケーションクラス。ウィンドウのタイトルと最初のシーンを決める。
/// シーンの更新・描画はエンジン（SceneManager）が行う。
/// </summary>
class MyGame : public Engine::Framework {
protected:

    void Initialize() override;
};
