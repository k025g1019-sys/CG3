#pragma once

#include "Engine/Rendering/Object3D.h"
#include "Game/Object/Hazard.h"

/// <summary>
/// ボスの周りを回る浮遊ビット。触れるとダメージ。プレイヤーの近接攻撃（空中攻撃の大きい円でまとめて）やブーメランで壊せる。
/// 回る中心はボスから毎フレーム SetAnchor で受け取る。
/// </summary>
class Bit : public Hazard {
public:

	/// <summary>
	/// 回り始める角度を決める
	/// </summary>
	/// <param name="anchor">回る中心（ボスの位置）</param>
	/// <param name="startAngle">開始角度（ラジアン）</param>
	void Initialize(const Engine::Vector3& anchor, float startAngle);

	void Update() override;

	void Draw() const override;

	Engine::Sphere GetHitSphere() const override { return { position_, kRadius }; }

	bool CanBeDestroyed() const override { return true; }

	void OnHitByPlayer() override { Kill(); }

	void SetAnchor(const Engine::Vector3& anchor) override { anchor_ = anchor; }

private:

	// --- 調整値 ---
	static inline float kOrbitRadius = 3.2f;    // 回る半径
	static inline float kAngularSpeed = 0.03f;  // 回る速さ（ラジアン / フレーム）
	static inline float kRadius = 0.35f;        // 当たり判定の半径

	Engine::Object3D model_;                           // 見た目（仮モデル：球）
	Engine::Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };    // 回る中心
	Engine::Vector3 position_ = { 0.0f, 0.0f, 0.0f };  // 現在位置
	float angle_ = 0.0f;                               // 現在の角度（ラジアン）
};
