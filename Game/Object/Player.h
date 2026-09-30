#pragma once

#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"

/// <summary>
/// プレイヤー（仮）。A/Dで左右に移動し、Wでジャンプする。
/// 見た目は組み込みの立方体（仮モデル）。
/// </summary>
class Player {
public:

    // 見た目（仮モデル）と初期位置を設定する
    void Initialize();

    // 入力による移動・ジャンプ（毎フレーム呼ぶ）
    void Update();

    void Draw() const;

#ifdef USE_IMGUI
    // 位置とジャンプの状態を表示する（呼び出し側のImGuiウィンドウの中に差し込む）
    void DrawImGui();
#endif

    const Engine::Vector3& GetPosition() const { return model_.GetTransform().translate; }

private:

    // --- 移動・ジャンプの調整値（1フレームあたりの量。更新は毎秒60回に固定されている）---
    static constexpr float kSpeed = 0.05f;      // 左右移動の速さ
    static constexpr float kJumpPower = 0.18f;  // ジャンプの初速
    static constexpr float kGravity = 0.01f;    // 重力（毎フレーム上向きの速度から引く）
    static constexpr float kGroundY = 0.5f;     // 着地する高さ（地面の上面が0で、立方体の中心は高さの半分）

    Engine::Object3D model_;     // 見た目（仮モデル：立方体）

    // --- ジャンプ用変数 ---
    float jumpVelocity_ = 0.0f;  // ジャンプ中の上向きの速度
    bool isGrounded_ = true;     // 地面に着いているか
};
