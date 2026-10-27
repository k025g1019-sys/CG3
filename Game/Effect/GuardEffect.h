#pragma once

#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Effect/BaseEffect.h"

/// <summary>
/// 盾で弾いたときのエフェクト。盾の位置の板と、弾いた点の円盤が広がりながら薄くなる。
/// 板は盾の向きに合わせて、縦長（正面に構えた盾）か横長（眠っているボスが体の下に構えた盾）にする
/// </summary>
class GuardEffect : public BaseEffect {
public:

	void Initialize(const Engine::Vector3& position) override;

	// 板を横長にするか（既定は縦長。Initialize の後、最初の Update の前に呼ぶ）
	void SetHorizontal(bool isHorizontal) { isHorizontal_ = isHorizontal; }

	// 板を描くか（既定は描く。地面に弾かれたときは円盤だけにする）
	void SetPlateVisible(bool isPlateVisible) { isPlateVisible_ = isPlateVisible; }

	// 全体の大きさの倍率（既定は 1。地面に弾かれたときは小さめにする。Initialize の後、最初の Update の前に呼ぶ）
	void SetSizeScale(float sizeScale) { sizeScale_ = sizeScale; }

	// 円盤だけを別の位置（弾いた点）に出す（既定は板と同じ位置。Initialize の後、最初の Update の前に呼ぶ）
	void SetDiscPosition(const Engine::Vector3& position) { disc_.GetTransform().translate = position; }

	void Update() override;

	void Draw() const override;

private:

	static inline const float kDuration = 0.25f;     // 消えるまでの時間（秒）
	static inline const float kStartScale = 0.8f;    // 最初の大きさの倍率
	static inline const float kEndScale = 1.4f;      // 最後の大きさの倍率
	static inline const float kPlateWidth = 0.15f;   // 板の厚み（縦長なら X、横長なら Y）
	static inline const float kPlateHeight = 2.6f;   // 板の長さ（縦長なら Y、横長なら X）
	static inline const float kPlateDepth = 2.2f;    // 板の奥行き（Z）
	static inline const float kDiscSize = 1.2f;      // 円盤の直径
	static inline const float kMaxAlpha = 0.7f;      // 最初の不透明度

	Engine::Object3D plate_;  // 盾に重なる板
	Engine::Object3D disc_;   // 弾いた点の円盤
	bool isHorizontal_ = false;  // 板を横長にするか
	float sizeScale_ = 1.0f;     // 全体の大きさの倍率
	bool isPlateVisible_ = true; // 板を描くか
};
