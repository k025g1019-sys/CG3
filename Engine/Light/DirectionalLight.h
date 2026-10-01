#pragma once

#include <cstdint>

#include "Engine/Math/Color.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Math/Vector3.h"

namespace Engine {

// 平行光源。HLSL側（Object3d.hlsli）と同じメモリレイアウトにすること。
struct DirectionalLight {
    Vector4 color;
    Vector3 direction;
    float intensity;
    int32_t enabled;      // 0:無効 / 非0:有効
    float padding[3];

    // 光の色を設定する（Vector4 または 0xRRGGBBAA の16進数）
    void SetColor(const Vector4& value) { color = value; }
    void SetColor(uint32_t rgba) { color = ColorFromHex(rgba); }
};

} // namespace Engine
