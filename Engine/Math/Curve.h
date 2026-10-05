#pragma once

#include <array>

#include "Engine/Math/Vector3.h"

namespace Engine {

/// <summary>
/// 2次ベジェ曲線上の点を求める（制御点3つ。t=0でp0、t=1でp2を通り、p1の方へ引っぱられる）
/// </summary>
/// <param name="p0">始点</param>
/// <param name="p1">中間の制御点</param>
/// <param name="p2">終点</param>
/// <param name="t">曲線上の位置（0.0〜1.0）</param>
Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t);

/// <summary>
/// 2次ベジェ曲線（制御点3つ）。My_Math の Curve を移植。
/// GetPoint(t) で曲線上の位置を取り、オブジェクトを曲線に沿って動かすのに使う。
/// 形の確認は DebugDraw::DrawCurve、制御点の調整は DrawImGui で行える。
/// </summary>
class Curve {
public:

    Curve() = default;

    Curve(const Vector3& p0, const Vector3& p1, const Vector3& p2)
        : controlPoints_{ p0, p1, p2 } {}

    // 曲線上の点（t: 0.0で始点、1.0で終点）
    Vector3 GetPoint(float t) const;

    const std::array<Vector3, 3>& GetControlPoints() const { return controlPoints_; }

    // 制御点を設定する（index: 0=始点 / 1=中間 / 2=終点）
    void SetControlPoint(int index, const Vector3& point);

#ifdef USE_IMGUI
    // 制御点を編集するUI（呼び出し側のImGuiウィンドウの中に差し込む）
    void DrawImGui(const char* label);
#endif

private:

    std::array<Vector3, 3> controlPoints_ = {
        Vector3{ -0.8f, 0.58f, 1.0f },
        Vector3{ 1.76f, 1.0f, -0.3f },
        Vector3{ 0.94f, -0.7f, 2.3f },
    };
};

} // namespace Engine
