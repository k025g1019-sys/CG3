#pragma once

#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector3.h"

// =============================================================
// 当たり判定（Collision.h）と視錐台カリング（FrustumCulling.h）で共通に使う形状。
//
// 平面（2Dでは直線）はすべて「単位法線 normal と距離 distance」で保持し、
// 符号付き距離 dot(normal, point) + distance が 0 の点の集まりを平面とする
// （視錐台では 0 以上を内側として使う）。
// 法線と平面上の1点から作るときは MakePlane（Collision.h）を使う。
// =============================================================

namespace Engine {

// ===================== 3D の形状 =====================

// 球
struct Sphere {
    Vector3 center;
    float radius;
};

// 軸並行境界ボックス（3D）
struct AABB3D {
    Vector3 min;
    Vector3 max;
};

// 有向境界ボックス（3D）。orientations は正規直交な3軸、size は各軸方向の半幅。
// 回転（ラジアン）から作るときは MakeOBB（Collision.h）を使う。
struct OBB3D {
    Vector3 center;
    Vector3 orientations[3];
    Vector3 size;
};

// 線分（3D）
struct Segment3D {
    Vector3 start;
    Vector3 end;
};

// 三角形（3D）
struct Triangle3D {
    Vector3 v0;
    Vector3 v1;
    Vector3 v2;
};

// カプセル（線分を半径ぶん太らせた形。移動する球の通り道などに使う）
struct Capsule3D {
    Segment3D segment;
    float radius;
};

// ===================== 2D の形状 =====================

// 円
struct Circle {
    Vector2 center;
    float radius;
};

// 軸並行境界ボックス（2D）
struct AABB2D {
    Vector2 min;
    Vector2 max;
};

// 有向境界ボックス（2D）。orientations は正規直交な2軸、size は各軸方向の半幅。
struct OBB2D {
    Vector2 center;
    Vector2 orientations[2];
    Vector2 size;
};

// 線分（2D）
struct Segment2D {
    Vector2 start;
    Vector2 end;
};

// 三角形（2D）
struct Triangle2D {
    Vector2 v0;
    Vector2 v1;
    Vector2 v2;
};

// ===================== 平面 / 直線 =====================

// 3Dの平面。dot(normal, p) + distance = 0 の点の集まり（視錐台では >= 0 が内側）。normal は単位ベクトル。
struct Plane3D {
    Vector3 normal;
    float distance;
};

// 2Dの直線（2Dにおける「平面」）。dot(normal, p) + distance >= 0 が内側。
struct Plane2D {
    Vector2 normal;
    float distance;
};

} // namespace Engine
