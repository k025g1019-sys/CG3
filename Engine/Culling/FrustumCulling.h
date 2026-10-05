#pragma once

#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Math/Matrix4x4.h"
#include "Engine/Math/Shapes.h"

// =============================================================
// 視錐台カリング用の視錐台と、形状（Shapes.h）との交差判定。
//
// ・3Dの視錐台は ViewProjection 行列から6平面を抽出（Gribb–Hartmann法）。
// ・2Dの視錐台は可視範囲の矩形（min/max）から4直線を生成する。
// ・判定は Inside / Intersect / Outside の3状態を返す。
//   描画スキップだけが目的なら IsVisible()（Outside以外）を使う。
//
// 平面（2Dでは直線）はすべて「内向き単位法線 normal と距離 distance」で保持し、
// 符号付き距離 dot(normal, point) + distance が 0 以上なら内側、と統一する。
//
// 形状の判定方針（保守的・軽量）:
//   ・円 / 球        : 中心の符号付き距離と半径で厳密に判定。
//   ・それ以外（箱・線分・三角形）: 形状の頂点集合を使い、
//        「ある1平面の外側に全頂点がある」→Outside /
//        「全平面で全頂点が内側」→Inside / それ以外→Intersect。
//     視錐台のコーナー付近でまれに内側と誤るが、見えるものを消すことはない（安全側）。
// =============================================================

namespace Engine {

// ---- 判定結果（3状態）----
enum class FrustumVisibility {
    Outside,    // 完全に視錐台の外（描画不要）
    Intersect,  // 境界をまたぐ（一部が内側）
    Inside,     // 完全に視錐台の内側
};

// ===================== 視錐台 =====================

// 3Dの視錐台。left / right / bottom / top / near / far の6平面。
struct Frustum3D {
    Plane3D planes[6];
};

// 2Dの視錐台（可視矩形）。left / right / bottom / top の4直線。
struct Frustum2D {
    Plane2D planes[4];
};

// ===================== 視錐台の生成 =====================

// ViewProjection 行列（world * view * projection の view * projection 部分）から
// 3D視錐台の6平面を抽出する。行ベクトル規約・DirectX（深度0..1）前提。
Frustum3D MakeFrustumFromViewProjection(const Matrix4x4& viewProjection);

// 可視範囲の矩形（min / max）から2D視錐台（4直線）を生成する。
Frustum2D MakeFrustumFromRect(const Vector2& min, const Vector2& max);

// ===================== 判定（3D）=====================
FrustumVisibility ClassifyFrustum(const Frustum3D& frustum, const Sphere& sphere);
FrustumVisibility ClassifyFrustum(const Frustum3D& frustum, const AABB3D& aabb);
FrustumVisibility ClassifyFrustum(const Frustum3D& frustum, const OBB3D& obb);
FrustumVisibility ClassifyFrustum(const Frustum3D& frustum, const Segment3D& segment);
FrustumVisibility ClassifyFrustum(const Frustum3D& frustum, const Triangle3D& triangle);

// ===================== 判定（2D）=====================
FrustumVisibility ClassifyFrustum(const Frustum2D& frustum, const Circle& circle);
FrustumVisibility ClassifyFrustum(const Frustum2D& frustum, const AABB2D& aabb);
FrustumVisibility ClassifyFrustum(const Frustum2D& frustum, const OBB2D& obb);
FrustumVisibility ClassifyFrustum(const Frustum2D& frustum, const Segment2D& segment);
FrustumVisibility ClassifyFrustum(const Frustum2D& frustum, const Triangle2D& triangle);

// ---- 可視判定ヘルパー（Outside以外＝描画すべき なら true）----
inline bool IsVisible(FrustumVisibility visibility) {
    return visibility != FrustumVisibility::Outside;
}

} // namespace Engine
