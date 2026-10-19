#pragma once

#include <cstdint>

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Math/Vector4.h"

namespace Engine {

/// <summary>
/// パーティクルの発生源（エミッタ）。どこから・どのくらいの頻度で・どんなパーティクルを出すかの設定と、
/// 発生頻度を計るタイマーを持つ（パーティクルそのものはParticleSystemが持つ）。
///   毎フレーム Update() を呼び、trueが返ったフレームに ParticleSystem::Emit(emitter) で発生させる。
/// 位置・速度・色は「基準の値 ± 幅」の範囲から乱数で決まる（幅が0なら、全パーティクルが基準の値になる）。
/// 1つのParticleSystemに対して、複数のエミッタから発生させてもよい。
/// </summary>
struct ParticleEmitter {

    // --- 発生する場所 ---

    Vector3 translate{ 0.0f, 0.0f, 0.0f };  // 発生の基準となる位置
    Vector3 range{ 1.0f, 1.0f, 1.0f };      // 発生範囲（基準となる位置から、各軸 ±range の箱の中に発生する）

    // --- 発生する数と頻度 ---

    uint32_t count = 3;          // 1回あたりの発生数
    float frequency = 0.5f;      // 発生頻度（秒。この時間が経つごとに1回発生する）
    float frequencyTime = 0.0f;  // 発生頻度計測用タイマー（前回の発生からの経過時間。秒）

    // --- 発生するパーティクルの設定 ---

    Vector3 velocity{ 0.0f, 0.0f, 0.0f };       // パーティクルの速度の基準（1秒あたりの移動量）
    Vector3 velocityRange{ 1.0f, 1.0f, 1.0f };  // 速度の幅（基準 ± この幅でランダムに決まる）

    float lifeTimeMin = 1.0f;  // パーティクルの生存時間の最小（秒）
    float lifeTimeMax = 3.0f;  // パーティクルの生存時間の最大（秒。最小～最大の間でランダムに決まる）

    Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };       // 初期色の基準（RGBA）
    Vector4 colorRange{ 0.0f, 0.0f, 0.0f, 0.0f };  // 初期色の幅（基準 ± この幅でランダムに決まる。0なら全て基準の色）

    // --- 関数 ---

    // 発生頻度計測用タイマーを進める（毎フレーム呼ぶ）。
    // 発生するタイミングになったフレームだけtrueを返す（trueならParticleSystem::Emitで発生させる）
    bool Update();

    // 発生範囲の箱（範囲表示用。DebugDraw::DrawAABBにそのまま渡せる）
    AABB3D CalcRangeAABB() const;
};

} // namespace Engine
