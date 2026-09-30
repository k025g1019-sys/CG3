#pragma once

#include "Engine/Rendering/Object3D.h"
#include "Engine/Scene/BaseScene.h"

/// <summary>
/// タイトル画面（仮）。Enter / Space / パッドのAボタンでゲーム画面へ進む。
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

    Engine::Object3D logo_;  // タイトルロゴの代わりの仮モデル（回転する立方体）
};
