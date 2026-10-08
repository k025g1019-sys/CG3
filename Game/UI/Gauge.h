#pragma once

#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Rendering/Sprite.h"

/// <summary>
/// 横に伸び縮みする HP ゲージ。暗い背景の上に残量のバーを重ね、減った分は白いバーが少し遅れて追いかける。
/// 位置と大きさは 1280x720 基準のピクセルで、左上の角を基準にする
/// </summary>
class Gauge {
public:

	/// <summary>
	/// 背景・遅れバー・残量バーのスプライトを作る（残量は満タンから始まる）
	/// </summary>
	/// <param name="position">左上の位置（px）</param>
	/// <param name="size">バーの大きさ（px）</param>
	/// <param name="barColor">残量バーの色</param>
	void Initialize(const Engine::Vector2& position, const Engine::Vector2& size, const Engine::Vector4& barColor);

	/// <summary>
	/// 残量の割合を反映する（毎フレーム呼ぶ）
	/// </summary>
	/// <param name="ratio">残量の割合（0〜1）</param>
	void Update(float ratio);

	/// <summary>
	/// 描く（OnDraw の最後、3D をすべて描いた後に呼ぶ）
	/// </summary>
	void Draw() const;

private:

	static inline float kDelayFollowRate = 0.08f;  // 遅れバーが残量バーに追いつく割合（1 フレームあたり）
	static inline const float kBorder = 3.0f;      // 背景をバーより広げる幅（px。枠に見せる）

	Engine::Sprite background_;  // 背景（暗い板。バーより一回り大きい）
	Engine::Sprite delayBar_;    // 遅れて減る白いバー
	Engine::Sprite bar_;         // 残量のバー

	Engine::Vector2 position_ = { 0.0f, 0.0f };  // 左上の位置
	Engine::Vector2 size_ = { 0.0f, 0.0f };      // バーの大きさ
	float delayRatio_ = 1.0f;                    // 遅れバーが表示している割合
};
