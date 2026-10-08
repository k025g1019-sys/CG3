#include "Game/Util/ScreenToWorld.h"

#include <cmath>

using namespace Engine;

Vector3 ScreenToWorldOnGamePlane(const Vector2& screenPosition, float renderWidth, float renderHeight, const Matrix4x4& viewProjection) {
	// スクリーン座標 → NDC（X は -1〜1、Y は上が +1。スクリーンは下が + なので反転する）
	const float ndcX = (screenPosition.x / renderWidth) * 2.0f - 1.0f;
	const float ndcY = 1.0f - (screenPosition.y / renderHeight) * 2.0f;

	// NDC の近い側（深度 0）と遠い側（深度 1）の点をワールドへ戻す（Transform は w 除算まで行う）
	const Matrix4x4 inverseViewProjection = Inverse(viewProjection);
	const Vector3 nearPoint = Transform({ ndcX, ndcY, 0.0f }, inverseViewProjection);
	const Vector3 farPoint = Transform({ ndcX, ndcY, 1.0f }, inverseViewProjection);

	// 2 点を結ぶ線分と平面 Z=0 の交点（t は近い側から平面までの割合）
	const Vector3 direction = farPoint - nearPoint;
	if (std::fabs(direction.z) < 1e-6f) {
		// 視線が平面と平行（横視点では起きない）。近い側の点をそのまま返す
		return { nearPoint.x, nearPoint.y, 0.0f };
	}
	const float t = -nearPoint.z / direction.z;
	Vector3 result = nearPoint + direction * t;
	result.z = 0.0f;
	return result;
}
