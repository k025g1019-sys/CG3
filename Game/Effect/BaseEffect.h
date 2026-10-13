#pragma once

#include "Engine/Math/Vector3.h"

/// <summary>
/// 使い捨てのエフェクトの基底クラス。発生位置で始まり、時間で進んで終わる。
/// 終わったものはシーンが取り除く（AL3 の BaseEffect と同じ形）
/// </summary>
class BaseEffect {
public:

	virtual ~BaseEffect() = default;

	/// <summary>
	/// 発生位置を受け取って始める（派生側で見た目を作る）
	/// </summary>
	/// <param name="position">発生位置</param>
	virtual void Initialize(const Engine::Vector3& position) = 0;

	/// <summary>
	/// 進める（毎フレーム呼ぶ。描くものは毎フレーム Update が必要）
	/// </summary>
	virtual void Update() = 0;

	/// <summary>
	/// 描く（半透明なので、不透明なものをすべて描いた後に呼ぶ）
	/// </summary>
	virtual void Draw() const = 0;

	bool IsFinished() const { return isFinished_; }

protected:

	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数

	Engine::Vector3 position_ = { 0.0f, 0.0f, 0.0f };  // 発生位置
	float counter_ = 0.0f;                             // 経過時間（秒）
	bool isFinished_ = false;                          // 終わったか
};
