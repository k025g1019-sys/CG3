#pragma once

#include "Engine/Math/Vector3.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Effect/BaseEffect.h"

#include <array>
#include <cstdint>

/// <summary>
/// ヒットエフェクト。当たった位置で円盤が広がり、火花が斜め 4 方向へ飛んで、薄くなって消える
/// </summary>
class HitEffect : public BaseEffect {
public:

	/// <summary>
	/// 既定の色（黄色）で始める
	/// </summary>
	void Initialize(const Engine::Vector3& position) override;

	/// <summary>
	/// 色を指定して始める（敵へのヒットは黄色、プレイヤーの被弾は赤 など）
	/// </summary>
	/// <param name="position">発生位置</param>
	/// <param name="color">色（アルファは進行に合わせて下げるので 1 でよい）</param>
	void Initialize(const Engine::Vector3& position, const Engine::Vector4& color);

	void Update() override;

	void Draw() const override;

private:

	static inline const uint32_t kNumSparks = 4;          // 火花の数（斜め 4 方向へ 1 つずつ）
	static inline const float kExpandDuration = 0.12f;    // 円盤が広がる時間（秒）
	static inline const float kFadeDuration = 0.18f;      // 広がった後、薄くなって消える時間（秒）
	static inline const float kStartSize = 0.4f;          // 円盤の最初の直径
	static inline const float kEndSize = 1.6f;            // 円盤の最後の直径
	static inline const float kThickness = 0.1f;          // 円盤の厚み（Z 方向）
	static inline const float kSparkSize = 0.18f;         // 火花の大きさ
	static inline const float kSparkSpeed = 0.12f;        // 火花の初速（1 フレームあたり）
	static inline const float kSparkAttenuation = 0.08f;  // 火花の減速率（1 フレームでこの割合だけ遅くなる）
	static inline const float kSparkSpinSpeed = 0.3f;     // 火花の回転速度（ラジアン / フレーム）
	static inline const float kMaxAlpha = 0.8f;           // 最初の不透明度

	Engine::Object3D disc_;                                  // 円盤（平たくした球）
	std::array<Engine::Object3D, kNumSparks> sparks_;        // 火花（小さい立方体）
	std::array<Engine::Vector3, kNumSparks> sparkVelocities_;  // 火花の速度（1 フレームあたり）
	Engine::Vector4 color_ = { 1.0f, 0.95f, 0.6f, 1.0f };    // 色（w は進行に合わせて下げる）
};
