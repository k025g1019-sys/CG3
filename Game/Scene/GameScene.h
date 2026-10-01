#pragma once

#include "Engine/Rendering/Object3D.h"
#include "Engine/Scene/BaseScene.h"
#include "Game/Object/Player.h"
#include "Game/Object/Enemy.h"

/// <summary>
/// ゲーム画面（仮）。地面の上でプレイヤーを動かす（A/D：移動、W：ジャンプ）。Escでタイトルへ戻る。
/// </summary>
class GameScene : public Engine::BaseScene {
protected:

    void OnInitialize() override;

    void OnUpdate() override;

    void OnDraw() override;

#ifdef USE_IMGUI
    void OnDrawImGui() override;
#endif

private:

    Player player_;
    Enemy enemy_;

    Engine::Object3D ground_;  // 地面（仮モデル：平面を広げて使う）
};
