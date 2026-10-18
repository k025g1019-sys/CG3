#pragma once

#include "Engine/Math/Vector3.h"

/// <summary>
/// ヒットストップ（攻撃が当たった瞬間に少しだけ動きを止める演出）と、その間の揺れ。
/// 持ち主（プレイヤー・ボス）が Start で始め、止まっている間は毎フレーム Update を呼ぶ。
/// IsActive の間は自分の動きを止め、GetShakeOffset の分だけ見た目の位置をずらす（判定の位置は動かさない）。
/// </summary>
class HitStop {
public:

	/// <summary>
	/// ヒットストップを始める（止まっている途中で呼ぶと、新しい時間で始め直す）
	/// </summary>
	/// <param name="duration">止める時間（秒）</param>
	/// <param name="shakeAmplitude">揺れ幅（0 なら止まるだけで揺れない）</param>
	void Start(float duration, float shakeAmplitude);

	/// <summary>
	/// 残り時間を減らす（止まっている間、毎フレーム呼ぶ）
	/// </summary>
	void Update();

	/// <summary>
	/// すぐにやめる（やり直しで最初の状態に戻すときなど）
	/// </summary>
	void Stop();

	// 止まっている最中か
	bool IsActive() const { return timer_ > 0.0f; }

	// 残り時間（秒）
	float GetRemainingTime() const { return timer_; }

	/// <summary>
	/// 見た目をずらす量（左右に細かく揺れ、終わりにかけて小さくなる。止まっていないときは 0）
	/// </summary>
	Engine::Vector3 GetShakeOffset() const;

private:

	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数
	static inline const float kShakeSpeed = 90.0f;        // 揺れの速さ（ラジアン / 秒。およそ 4 フレームで 1 往復）

	float timer_ = 0.0f;           // 残り時間（秒）
	float duration_ = 0.0f;        // 今回止める時間（秒）
	float shakeAmplitude_ = 0.0f;  // 今回の揺れ幅
};
