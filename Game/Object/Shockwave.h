#pragma once

#include "Engine/Rendering/Object3D.h"
#include "Game/Object/Hazard.h"

class Stage;

/// <summary>
/// 叩きつけで生まれる衝撃波。床の上を一方向に進み、ステージの端で消える。低いのでジャンプで飛び越えられる。
/// </summary>
class Shockwave : public Hazard {
public:

	/// <summary>
	/// 出発点と進む向きを決める
	/// </summary>
	/// <param name="position">出発点（床の高さは内部で合わせる）</param>
	/// <param name="direction">進む向き（+1 で右、-1 で左）</param>
	/// <param name="stage">端の座標を参照するステージ（所有しない）</param>
	void Initialize(const Engine::Vector3& position, float direction, const Stage* stage);

	void Update() override;

	void Draw() const override;

	Engine::Sphere GetHitSphere() const override { return { position_, kRadius }; }

private:

	// --- 調整値 ---
	static inline float kSpeed = 0.22f;   // 進む速さ（1 フレームあたり）
	static inline float kRadius = 0.45f;  // 当たり判定の半径（中心は床から半径ぶん上）

	Engine::Object3D model_;                           // 見た目（仮モデル：平たい立方体）
	Engine::Vector3 position_ = { 0.0f, 0.0f, 0.0f };  // 現在位置
	float direction_ = 1.0f;                           // 進む向き
	const Stage* stage_ = nullptr;                     // 端の座標（借りるだけ）
};
