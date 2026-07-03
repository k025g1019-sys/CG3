#pragma once

#include <cstdint>

#include "Engine/Math/Vector4.h"
#include "Engine/Math/Vector3.h"

// 平行光源。HLSL側（Object3d.hlsli）と同じメモリレイアウトにすること。
struct DirectionalLight {
    Vector4 color;
    Vector3 direction;
    float intensity;
    int32_t enabled;      // 0:無効 / 非0:有効
    float padding[3];
};
