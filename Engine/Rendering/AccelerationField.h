#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"

namespace Engine {

/// <summary>
/// パーティクルに影響を与える場（Field）のうち、範囲内のパーティクルを加速させる場（風など）。
/// 範囲（AABB）の中にいるパーティクルだけ、毎フレーム「加速度 × 経過時間」ぶん速度が変わる。
///   毎フレーム ParticleSystem::ApplyField(field) を ParticleSystem::Update より前に呼んで適用する。
///   範囲は DebugDraw::DrawAABB(field.area) でそのまま表示できる。
/// 1つのParticleSystemに対して、複数の場を適用してもよい。
/// </summary>
struct AccelerationField {
    Vector3 acceleration{ 0.0f, 0.0f, 0.0f };  // 加速度（1秒あたりの速度の変化量）
    AABB3D area{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } };  // 範囲（この中にいるパーティクルだけ加速させる）
};

} // namespace Engine
