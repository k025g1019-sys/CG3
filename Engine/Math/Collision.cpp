#include "Engine/Math/Collision.h"

#include <cmath>

#include "Engine/Math/Matrix4x4.h"

namespace Engine {

namespace {

// 平行判定・0除算防止に使う小さな値
constexpr float kEpsilon = 1e-6f;

// min / max / clamp（std::min等はWindows.hのmin/maxマクロと衝突するため自前で用意する）
float MinF(float a, float b) { return (a < b) ? a : b; }
float MaxF(float a, float b) { return (a > b) ? a : b; }
float ClampF(float value, float low, float high) { return (value < low) ? low : ((value > high) ? high : value); }

// ベクトルの成分をインデックスで取り出す（0:x / 1:y / 2:z）
float Component(const Vector3& v, int index) {
	return (index == 0) ? v.x : ((index == 1) ? v.y : v.z);
}

// AABBを、軸が座標軸に平行なOBBに変換する
OBB3D ToOBB(const AABB3D& aabb) {
	OBB3D obb{};
	obb.center = (aabb.min + aabb.max) * 0.5f;
	obb.size = (aabb.max - aabb.min) * 0.5f;
	obb.orientations[0] = { 1.0f, 0.0f, 0.0f };
	obb.orientations[1] = { 0.0f, 1.0f, 0.0f };
	obb.orientations[2] = { 0.0f, 0.0f, 1.0f };
	return obb;
}

}  // namespace

// ===================== 形状を作る補助関数 =====================

AABB3D MakeAABB(const Vector3& center, const Vector3& halfSize) {
	return { center - halfSize, center + halfSize };
}

OBB3D MakeOBB(const Vector3& center, const Vector3& rotate, const Vector3& halfSize) {
	// Object3Dと同じ回転行列を作ると、その各行が回転後のローカル軸（X/Y/Z）になる
	const Matrix4x4 rotateMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate, { 0.0f, 0.0f, 0.0f });

	OBB3D obb{};
	obb.center = center;
	for (int i = 0; i < 3; ++i) {
		obb.orientations[i] = Normalize({ rotateMatrix.m[i][0], rotateMatrix.m[i][1], rotateMatrix.m[i][2] });
	}
	// 半サイズは正に
	obb.size = { std::fabs(halfSize.x), std::fabs(halfSize.y), std::fabs(halfSize.z) };
	return obb;
}

Plane3D MakePlane(const Vector3& normal, const Vector3& point) {
	const Vector3 unitNormal = Normalize(normal);
	return { unitNormal, -Dot(unitNormal, point) };
}

float SignedDistance(const Vector3& point, const Plane3D& plane) {
	return Dot(plane.normal, point) + plane.distance;
}

Vector3 ClosestPoint(const Vector3& point, const Segment3D& segment) {
	const Vector3 diff = segment.end - segment.start;
	const float lengthSq = Dot(diff, diff);
	if (lengthSq < kEpsilon) {
		return segment.start;  // 長さ0の線分
	}

	// 線分なので 0～1 にクランプ
	const float t = ClampF(Dot(point - segment.start, diff) / lengthSq, 0.0f, 1.0f);
	return segment.start + diff * t;
}

// ===================== 当たり判定 =====================

// 球同士：中心間の距離が半径の和以下なら衝突
bool IsCollision(const Sphere& a, const Sphere& b) {
	const Vector3 diff = b.center - a.center;
	const float radiusSum = a.radius + b.radius;
	return Dot(diff, diff) <= radiusSum * radiusSum;
}

// 球と平面：球の中心から平面までの距離が半径以下なら衝突
bool IsCollision(const Sphere& sphere, const Plane3D& plane) {
	return std::fabs(SignedDistance(sphere.center, plane)) <= sphere.radius;
}

// カプセル(startからendへスイープした球)と平面
bool IsCollision(const Capsule3D& capsule, const Plane3D& plane) {
	// 両端点の平面までの符号付き距離
	const float startDistance = SignedDistance(capsule.segment.start, plane);
	const float endDistance = SignedDistance(capsule.segment.end, plane);

	// 符号が異なれば線分が平面を貫いている
	if (startDistance * endDistance <= 0.0f) {
		return true;
	}

	// 貫いていなければ、平面に近い方の端点との距離で判定
	return MinF(std::fabs(startDistance), std::fabs(endDistance)) <= capsule.radius;
}

// 線分と平面
bool IsCollision(const Segment3D& segment, const Plane3D& plane) {
	const Vector3 diff = segment.end - segment.start;

	// 垂直判定を行うために、法線と線の内積を求める
	const float dot = Dot(plane.normal, diff);

	// 平行なので、衝突しているはずがない
	if (dot == 0.0f) {
		return false;
	}

	// tを求め、線分の範囲（0～1）にあれば衝突
	const float t = -SignedDistance(segment.start, plane) / dot;
	return t >= 0.0f && t <= 1.0f;
}

// 三角形と線分
bool IsCollision(const Triangle3D& triangle, const Segment3D& segment) {
	const Vector3& v0 = triangle.v0;
	const Vector3& v1 = triangle.v1;
	const Vector3& v2 = triangle.v2;
	const Vector3 diff = segment.end - segment.start;

	// 三角形の法線
	const Vector3 normal = Normalize(Cross(v1 - v0, v2 - v0));

	// 平面との交差チェック（平行なら交差しない）
	const float denom = Dot(normal, diff);
	if (std::fabs(denom) < kEpsilon) {
		return false;
	}

	// tを求める（線分の範囲外なら交差しない）
	const float t = Dot(v0 - segment.start, normal) / denom;
	if (t < 0.0f || t > 1.0f) {
		return false;
	}

	// 交点
	const Vector3 p = segment.start + diff * t;

	// 各辺を結んだベクトルと、頂点と衝突点pを結んだベクトルのクロス積を取る
	const Vector3 cross01 = Cross(v1 - v0, p - v0);
	const Vector3 cross12 = Cross(v2 - v1, p - v1);
	const Vector3 cross20 = Cross(v0 - v2, p - v2);

	// すべての小三角形のクロス積と法線が同じ方向を向いていたら衝突
	return Dot(cross01, normal) >= 0.0f && Dot(cross12, normal) >= 0.0f && Dot(cross20, normal) >= 0.0f;
}

// AABB同士：全軸で範囲が重なっていれば衝突
bool IsCollision(const AABB3D& a, const AABB3D& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// AABBと球：球の中心に最も近いAABB上の点との距離が半径以下なら衝突
bool IsCollision(const AABB3D& aabb, const Sphere& sphere) {
	// 最近接点を求める
	const Vector3 closestPoint{
		ClampF(sphere.center.x, aabb.min.x, aabb.max.x),
		ClampF(sphere.center.y, aabb.min.y, aabb.max.y),
		ClampF(sphere.center.z, aabb.min.z, aabb.max.z),
	};
	// 最近接点と球の中心との距離が半径以下なら衝突
	const Vector3 diff = closestPoint - sphere.center;
	return Dot(diff, diff) <= sphere.radius * sphere.radius;
}

// AABBと線分（スラブ法）
bool IsCollision(const AABB3D& aabb, const Segment3D& segment) {
	const Vector3 diff = segment.end - segment.start;

	float tMin = 0.0f;
	float tMax = 1.0f;

	// 各軸ごとに処理
	for (int i = 0; i < 3; ++i) {
		const float start = Component(segment.start, i);
		const float direction = Component(diff, i);
		const float minB = Component(aabb.min, i);
		const float maxB = Component(aabb.max, i);

		if (std::fabs(direction) < kEpsilon) {
			// 線分がこの軸に平行：範囲外なら衝突しない
			if (start < minB || start > maxB) {
				return false;
			}
		} else {
			const float t1 = (minB - start) / direction;
			const float t2 = (maxB - start) / direction;

			// AABBとの衝突点(貫通点)のtが小さい方 / 大きい方
			tMin = MaxF(tMin, MinF(t1, t2));
			tMax = MinF(tMax, MaxF(t1, t2));

			if (tMin > tMax) {
				return false;  // 交差しない
			}
		}
	}

	// [0,1] 区間で交差していれば線分と衝突
	return true;
}

// OBBと球：球の中心をOBBのローカル軸へ射影して最近接点を求める
bool IsCollision(const OBB3D& obb, const Sphere& sphere) {
	// OBB中心 → 球中心ベクトル
	const Vector3 d = sphere.center - obb.center;

	Vector3 closest = obb.center;
	// 各ローカル軸方向に射影して、半サイズでクランプ
	for (int i = 0; i < 3; ++i) {
		const float halfSize = Component(obb.size, i);
		const float distance = ClampF(Dot(d, obb.orientations[i]), -halfSize, halfSize);
		closest = closest + obb.orientations[i] * distance;
	}

	// 最近接点 → 球中心の距離
	const Vector3 diff = sphere.center - closest;
	return Dot(diff, diff) <= sphere.radius * sphere.radius;
}

// OBBと線分：線分をOBBのローカル空間へ移し、AABBと線分の判定にする
bool IsCollision(const OBB3D& obb, const Segment3D& segment) {
	// OBB中心基準へ移動し、ローカル軸へ射影する
	const Vector3 localStart = segment.start - obb.center;
	const Vector3 localEnd = segment.end - obb.center;
	const Segment3D localSegment{
		{ Dot(localStart, obb.orientations[0]), Dot(localStart, obb.orientations[1]), Dot(localStart, obb.orientations[2]) },
		{ Dot(localEnd, obb.orientations[0]), Dot(localEnd, obb.orientations[1]), Dot(localEnd, obb.orientations[2]) },
	};

	// OBB → AABB化（ローカル空間では原点中心の箱）
	const AABB3D localAABB{ -obb.size, obb.size };

	return IsCollision(localAABB, localSegment);
}

// OBB同士（分離軸判定：Aの3軸・Bの3軸・外積の9軸）
bool IsCollision(const OBB3D& a, const OBB3D& b) {
	const Vector3* axisA = a.orientations;
	const Vector3* axisB = b.orientations;
	const float halfA[3] = { a.size.x, a.size.y, a.size.z };
	const float halfB[3] = { b.size.x, b.size.y, b.size.z };

	// 中心間ベクトルをA基準へ変換
	const Vector3 tWorld = b.center - a.center;
	const float t[3] = { Dot(tWorld, axisA[0]), Dot(tWorld, axisA[1]), Dot(tWorld, axisA[2]) };

	// Bの軸をA基準で表した回転行列（平行な軸の誤差対策にEPSILONを足しておく）
	float R[3][3];
	float AbsR[3][3];
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			R[i][j] = Dot(axisA[i], axisB[j]);
			AbsR[i][j] = std::fabs(R[i][j]) + kEpsilon;
		}
	}

	float ra = 0.0f;
	float rb = 0.0f;

	// --- Aの軸 3本 ---
	for (int i = 0; i < 3; ++i) {
		ra = halfA[i];
		rb = halfB[0] * AbsR[i][0] + halfB[1] * AbsR[i][1] + halfB[2] * AbsR[i][2];
		if (std::fabs(t[i]) > ra + rb) {
			return false;
		}
	}

	// --- Bの軸 3本 ---
	for (int j = 0; j < 3; ++j) {
		ra = halfA[0] * AbsR[0][j] + halfA[1] * AbsR[1][j] + halfA[2] * AbsR[2][j];
		rb = halfB[j];
		const float distance = std::fabs(t[0] * R[0][j] + t[1] * R[1][j] + t[2] * R[2][j]);
		if (distance > ra + rb) {
			return false;
		}
	}

	// --- 外積軸 9本 ---

	// A0 x B0
	ra = halfA[1] * AbsR[2][0] + halfA[2] * AbsR[1][0];
	rb = halfB[1] * AbsR[0][2] + halfB[2] * AbsR[0][1];
	if (std::fabs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) { return false; }

	// A0 x B1
	ra = halfA[1] * AbsR[2][1] + halfA[2] * AbsR[1][1];
	rb = halfB[0] * AbsR[0][2] + halfB[2] * AbsR[0][0];
	if (std::fabs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) { return false; }

	// A0 x B2
	ra = halfA[1] * AbsR[2][2] + halfA[2] * AbsR[1][2];
	rb = halfB[0] * AbsR[0][1] + halfB[1] * AbsR[0][0];
	if (std::fabs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) { return false; }

	// A1 x B0
	ra = halfA[0] * AbsR[2][0] + halfA[2] * AbsR[0][0];
	rb = halfB[1] * AbsR[1][2] + halfB[2] * AbsR[1][1];
	if (std::fabs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) { return false; }

	// A1 x B1
	ra = halfA[0] * AbsR[2][1] + halfA[2] * AbsR[0][1];
	rb = halfB[0] * AbsR[1][2] + halfB[2] * AbsR[1][0];
	if (std::fabs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) { return false; }

	// A1 x B2
	ra = halfA[0] * AbsR[2][2] + halfA[2] * AbsR[0][2];
	rb = halfB[0] * AbsR[1][1] + halfB[1] * AbsR[1][0];
	if (std::fabs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) { return false; }

	// A2 x B0
	ra = halfA[0] * AbsR[1][0] + halfA[1] * AbsR[0][0];
	rb = halfB[1] * AbsR[2][2] + halfB[2] * AbsR[2][1];
	if (std::fabs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) { return false; }

	// A2 x B1
	ra = halfA[0] * AbsR[1][1] + halfA[1] * AbsR[0][1];
	rb = halfB[0] * AbsR[2][2] + halfB[2] * AbsR[2][0];
	if (std::fabs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) { return false; }

	// A2 x B2
	ra = halfA[0] * AbsR[1][2] + halfA[1] * AbsR[0][2];
	rb = halfB[0] * AbsR[2][1] + halfB[1] * AbsR[2][0];
	if (std::fabs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) { return false; }

	// どの軸でも分離できなかったので衝突
	return true;
}

// OBBとAABB：AABBを回転していないOBBとみなして判定する
bool IsCollision(const OBB3D& obb, const AABB3D& aabb) {
	return IsCollision(obb, ToOBB(aabb));
}

} // namespace Engine
