#pragma once

#include <cstdint>

#include "Engine/Math/Vector4.h"
#include "Engine/Math/Vector3.h"

namespace Engine {

// 点光源。ワールド座標から全方向へ光を放ち、半径と減衰率で距離減衰する。
// HLSL側（Object3d.hlsli）と同じメモリレイアウトにすること。
struct PointLight {
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector3 position = { 0.0f, 2.0f, 0.0f };
    float intensity = 1.0f;
    float radius = 10.0f;    // 光が届く最大距離（この距離で減衰率0になる）
    float decay = 1.0f;      // 減衰カーブ（大きいほど光源の近くで急激に暗くなる）
    int32_t enabled = 0;     // 0:無効 / 非0:有効
    float padding = 0.0f;
};

// シーンに置ける点光源の最大数（Object3d.hlsliの配列サイズと一致させる）
inline constexpr uint32_t kMaxPointLightCount = 4;

// 全点光源をまとめてPS(b2)へ送るCBufferデータ
struct PointLightGroup {
    PointLight lights[kMaxPointLightCount];
};

} // namespace Engine
