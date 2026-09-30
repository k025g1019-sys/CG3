#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"

// =============================================================
// 3D形状同士の当たり判定（My_Math の Collision を移植）。
// どの判定も「接しているだけ」でも当たり（true）とする。
// 引数の順番はどちらでもよい（逆順の組み合わせも用意してある）。
//
// 平面は Shapes.h の Plane3D（dot(normal, p) + distance = 0）。
// My_Math の Plane(normal, d)（dot(normal, p) = d）は Plane3D{ normal, -d } にあたる。
// 法線と平面上の1点からなら MakePlane で作れる。
// =============================================================

namespace Engine {

// ===================== 形状を作る補助関数 =====================

// 中心と半分の大きさ（各軸方向の半幅）からAABBを作る
AABB3D MakeAABB(const Vector3& center, const Vector3& halfSize);

// 中心・回転（ラジアン。Object3Dと同じX→Y→Zの順）・半分の大きさからOBBを作る
OBB3D MakeOBB(const Vector3& center, const Vector3& rotate, const Vector3& halfSize);

// 法線と平面上の1点から平面を作る（法線は正規化される）
Plane3D MakePlane(const Vector3& normal, const Vector3& point);

// 点から平面までの符号付き距離（法線の向いている側が正）
float SignedDistance(const Vector3& point, const Plane3D& plane);

// 線分上で、指定した点に最も近い点
Vector3 ClosestPoint(const Vector3& point, const Segment3D& segment);

// ===================== 当たり判定 =====================

// 球 と 球
bool IsCollision(const Sphere& a, const Sphere& b);

// 球 と 平面
bool IsCollision(const Sphere& sphere, const Plane3D& plane);

// カプセル と 平面（移動した球が平面をすり抜けたかの判定などに使う）
bool IsCollision(const Capsule3D& capsule, const Plane3D& plane);

// 線分 と 平面
bool IsCollision(const Segment3D& segment, const Plane3D& plane);

// 三角形 と 線分
bool IsCollision(const Triangle3D& triangle, const Segment3D& segment);

// AABB と AABB
bool IsCollision(const AABB3D& a, const AABB3D& b);

// AABB と 球
bool IsCollision(const AABB3D& aabb, const Sphere& sphere);

// AABB と 線分
bool IsCollision(const AABB3D& aabb, const Segment3D& segment);

// OBB と 球
bool IsCollision(const OBB3D& obb, const Sphere& sphere);

// OBB と 線分
bool IsCollision(const OBB3D& obb, const Segment3D& segment);

// OBB と OBB（分離軸判定。15軸）
bool IsCollision(const OBB3D& a, const OBB3D& b);

// OBB と AABB
bool IsCollision(const OBB3D& obb, const AABB3D& aabb);

// --- 引数の順番を入れ替えた組み合わせ ---
inline bool IsCollision(const Plane3D& plane, const Sphere& sphere) { return IsCollision(sphere, plane); }
inline bool IsCollision(const Plane3D& plane, const Capsule3D& capsule) { return IsCollision(capsule, plane); }
inline bool IsCollision(const Plane3D& plane, const Segment3D& segment) { return IsCollision(segment, plane); }
inline bool IsCollision(const Segment3D& segment, const Triangle3D& triangle) { return IsCollision(triangle, segment); }
inline bool IsCollision(const Sphere& sphere, const AABB3D& aabb) { return IsCollision(aabb, sphere); }
inline bool IsCollision(const Segment3D& segment, const AABB3D& aabb) { return IsCollision(aabb, segment); }
inline bool IsCollision(const Sphere& sphere, const OBB3D& obb) { return IsCollision(obb, sphere); }
inline bool IsCollision(const Segment3D& segment, const OBB3D& obb) { return IsCollision(obb, segment); }
inline bool IsCollision(const AABB3D& aabb, const OBB3D& obb) { return IsCollision(obb, aabb); }

} // namespace Engine
