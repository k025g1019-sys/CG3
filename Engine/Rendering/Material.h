#pragma once
#include <cstdint>
#include "Engine/Math/Vector4.h"
#include "Engine/Math/Matrix4x4.h"

namespace Engine {

// ライティングの計算方式（シェーダー側Object3d.PS.hlslのkLightingMode定数と一致させる）
enum class LightingMode : int32_t {
    kNone = 0,         // ライティングなし（マテリアル×テクスチャの色をそのまま表示）
    kLambert = 1,      // ランバート反射（N・Lをそのまま使う。陰影の境界がはっきり出る）
    kHalfLambert = 2,  // ハーフランバート反射（N・Lを0..1へ写して2乗。陰が柔らかくなる）
};

struct Material {
    Vector4 color;
    LightingMode lightingMode;
    float padding[3];
    Matrix4x4 uvTransform;
};

} // namespace Engine
