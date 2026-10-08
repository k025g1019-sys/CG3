#pragma once

#include "Engine/Rendering/Object3D.h"
#include "Game/Object/Hazard.h"

class Stage;

/// <summary>
/// 上空から落ちてくる岩。床に着くと砕けて消える。プレイヤーの近接攻撃やブーメランで壊せる。
/// </summary>
class FallingRock : public Hazard {
public:

	/// <summary>
	/// 出現位置を決める
	/// </summary>
	/// <param name="position">出現位置（上空）</param>
	/// <param name="stage">床の高さを参照するステージ（所有しない）</param>
	void Initialize(const Engine::Vector3& position, const Stage* stage);

	void Update() override;

	void Draw() const override;

	Engine::Sphere GetHitSphere() const override { return { position_, kRadius }; }

	bool CanBeDestroyed() const override { return true; }

	void OnHitByPlayer() override { Kill(); }

private:

	// --- 調整値 ---
	static inline float kGravity = 0.012f;      // 落下の加速度（1 フレームあたり）
	static inline float kMaxFallSpeed = 0.4f;   // 落下速度の上限
	static inline float kRadius = 0.6f;         // 当たり判定の半径
	static inline float kSpinSpeed = 0.05f;     // 見た目の回転速度（ラジアン / フレーム）

	Engine::Object3D model_;                           // 見た目（仮モデル：球）
	Engine::Vector3 position_ = { 0.0f, 0.0f, 0.0f };  // 現在位置
	float fallSpeed_ = 0.0f;                           // 落下速度（下向きが正）
	const Stage* stage_ = nullptr;                     // 床の高さ（借りるだけ）
};
