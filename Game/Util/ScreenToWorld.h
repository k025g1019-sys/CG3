#pragma once

#include "Engine/Math/Matrix4x4.h"
#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector3.h"

/// <summary>
/// 描画先の左上を原点にしたスクリーン座標（px）を、ゲームの平面 Z=0 上のワールド座標に変換する。
/// ビュー射影の逆行列で近い側と遠い側の 2 点をワールドへ戻し、その線分と平面 Z=0 の交点を取る。
/// </summary>
/// <param name="screenPosition">描画先の左上を原点にしたスクリーン座標（px）</param>
/// <param name="renderWidth">描画先の幅（px）</param>
/// <param name="renderHeight">描画先の高さ（px）</param>
/// <param name="viewProjection">ビュー行列 × 射影行列</param>
/// <returns>Z=0 平面上のワールド座標</returns>
Engine::Vector3 ScreenToWorldOnGamePlane(const Engine::Vector2& screenPosition, float renderWidth, float renderHeight, const Engine::Matrix4x4& viewProjection);
