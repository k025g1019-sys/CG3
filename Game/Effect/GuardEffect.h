#pragma once

#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Effect/BaseEffect.h"

/// <summary>
/// 盾で弾いたときのエフェクト。盾の位置で縦長の板と円盤が広がりながら薄くなる
/// </summary>
class GuardEffect : public BaseEffect {
public:

	void Initialize(const Engine::Vector3& position) override;

	void Update() override;

	void Draw() const override;

private:

	static inline const float kDuration = 0.25f;     // 消えるまでの時間（秒）
	static inline const float kStartScale = 0.8f;    // 最初の大きさの倍率
	static inline const float kEndScale = 1.4f;      // 最後の大きさの倍率
	static inline const float kPlateWidth = 0.15f;   // 板の厚み（X）
	static inline const float kPlateHeight = 2.6f;   // 板の高さ（Y）
	static inline const float kPlateDepth = 2.2f;    // 板の奥行き（Z）
	static inline const float kDiscSize = 1.2f;      // 円盤の直径
	static inline const float kMaxAlpha = 0.7f;      // 最初の不透明度

	Engine::Object3D plate_;  // 盾に重なる縦長の板
	Engine::Object3D disc_;   // 弾いた点の円盤
};
