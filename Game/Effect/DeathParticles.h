#pragma once

#include "Engine/Math/Vector3.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Rendering/Object3D.h"

#include <array>
#include <cstdint>

/// <summary>
/// やられ演出のパーティクル。白い球が 8 方向へ飛び散り、薄くなって消える
/// </summary>
class DeathParticles {
public:

	/// <summary>
	/// 全パーティクルを発生位置にそろえて作る
	/// </summary>
	/// <param name="position">発生位置（プレイヤーがやられた位置）</param>
	void Initialize(const Engine::Vector3& position);

	/// <summary>
	/// 外へ広げながら薄くする（毎フレーム呼ぶ）
	/// </summary>
	void Update();

	/// <summary>
	/// 描く（半透明なので、不透明なものをすべて描いた後に呼ぶ）
	/// </summary>
	void Draw() const;

	/// <summary>
	/// 演出が終わったか（透明になった）
	/// </summary>
	bool IsFinished() const { return isFinished_; }

private:

	static inline const uint32_t kNumParticles = 8;        // 個数（8 方向へ 1 つずつ）
	static inline const float kFrameTime = 1.0f / 60.0f;   // 1 フレームの秒数
	static inline float kDuration = 1.0f;                  // 消えるまでの時間（秒）
	static inline float kSpeed = 0.1f;                     // 飛び散る速さ（1 フレームあたり）
	static inline float kScale = 0.5f;                     // 球の直径（プレイヤーの半分）

	std::array<Engine::Object3D, kNumParticles> particles_;  // 各パーティクル（仮モデル：球）
	Engine::Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };     // 全パーティクル共通の色（w のアルファを下げて消す）
	float counter_ = 0.0f;                                   // 経過時間（秒）
	bool isFinished_ = false;                                // 演出が終わったか
};
