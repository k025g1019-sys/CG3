#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"

/// <summary>
/// ボスが出す攻撃物（衝撃波・落石・浮遊ビット）の基底クラス。
/// 更新・描画・当たり判定の球を派生側が実装する。プレイヤーの攻撃で壊せるものは CanBeDestroyed を true にする。
/// </summary>
class Hazard {
public:

	virtual ~Hazard() = default;

	virtual void Update() = 0;

	virtual void Draw() const = 0;

	// 当たり判定（プレイヤーへのダメージと、プレイヤーの攻撃による破壊の両方に使う）
	virtual Engine::Sphere GetHitSphere() const = 0;

	// プレイヤーの攻撃で壊せるか
	virtual bool CanBeDestroyed() const { return false; }

	// プレイヤーの攻撃を受けた（壊せるものは消える）
	virtual void OnHitByPlayer() {}

	// 追従する基準点を受け取る（ボスの周りを回るビットが使う。他は無視）
	virtual void SetAnchor(const Engine::Vector3& /*anchor*/) {}

	bool IsAlive() const { return isAlive_; }

	// 消す（ボスが倒れたときなど）
	void Kill() { isAlive_ = false; }

protected:

	bool isAlive_ = true;  // 生きている間だけ更新・描画・判定の対象になる
};
