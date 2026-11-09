#pragma once
#include "Engine/Math/Matrix4x4.h"

namespace Engine {

// オブジェクトごとのワールド行列（VSのb0と一致させる）。
// ビュー射影は視点ごとの別CBuffer（VSのb1）で供給する（立体視の視点切り替え用）。
struct TransformationMatrix {
    Matrix4x4 World;
    // 法線変換用のワールド行列の逆転置行列（非均一スケールでも法線が面に垂直なまま保たれる）
    Matrix4x4 WorldInverseTranspose;
};

// ワールド行列から、逆転置行列も埋めた定数バッファ用データを作る
// （スケールが0で逆行列が作れないときは、Inverseの戻り値の単位行列になる）
inline TransformationMatrix MakeTransformationMatrix(const Matrix4x4& world) {
    return TransformationMatrix{ world, Transpose(Inverse(world)) };
}

} // namespace Engine
