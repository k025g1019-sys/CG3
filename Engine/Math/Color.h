#pragma once

#include <cstdint>

#include "Engine/Math/Vector4.h"

namespace Engine {

/// <summary>
/// 0xRRGGBBAA 形式の16進数を色（RGBA、各 0.0〜1.0）へ変換する（例: 0xFF6650FF）。
/// 各成分を 255 で割るだけで sRGB の補正はしない（ImGui の色編集欄の16進数と同じ値になる）。
/// 必ず8桁で書くこと（0xFF6650 のような6桁は 0x00FF6650 = 赤00・緑FF・青66・不透明度50 と解釈される）。
/// </summary>
constexpr Vector4 ColorFromHex(uint32_t rgba) {
    return {
        float((rgba >> 24) & 0xFF) / 255.0f,
        float((rgba >> 16) & 0xFF) / 255.0f,
        float((rgba >> 8) & 0xFF) / 255.0f,
        float(rgba & 0xFF) / 255.0f,
    };
}

} // namespace Engine
