#include "Game/Effect/GuardEffect.h"

#include "Game/Util/Easing.h"

#include <algorithm>

using namespace Engine;

namespace {

const Vector4 kGuardColor = { 0.6f, 0.85f, 1.0f, 1.0f };  // 弾いたときの色（水色）

} // namespace

void GuardEffect::Initialize(const Vector3& position) {
	position_ = position;
	counter_ = 0.0f;
	isFinished_ = false;

	// どちらもライティングなしで色をそのまま出す
	plate_.Initialize(Primitive::kCube, LightingMode::kNone);
	plate_.GetTransform().translate = position_;
	plate_.GetTransform().scale = { kPlateWidth, kPlateHeight, kPlateDepth };

	disc_.Initialize(Primitive::kSphere, LightingMode::kNone);
	disc_.GetTransform().translate = position_;
	disc_.GetTransform().scale = { kDiscSize, kDiscSize, 0.1f };
}

void GuardEffect::Update() {
	if (isFinished_) {
		return;
	}

	counter_ += kFrameTime;
	if (counter_ >= kDuration) {
		counter_ = kDuration;
		isFinished_ = true;
	}

	// 最初が速く広がりながら、まっすぐ薄くなる
	const float t = std::clamp(counter_ / kDuration, 0.0f, 1.0f);
	const float scale = kStartScale + (kEndScale - kStartScale) * EaseOutQuad(t);
	const Vector4 color = { kGuardColor.x, kGuardColor.y, kGuardColor.z, kMaxAlpha * (1.0f - t) };

	plate_.GetTransform().scale = { kPlateWidth, kPlateHeight * scale, kPlateDepth * scale };
	plate_.SetColor(color);
	plate_.Update();

	disc_.GetTransform().scale = { kDiscSize * scale, kDiscSize * scale, 0.1f };
	disc_.SetColor(color);
	disc_.Update();
}

void GuardEffect::Draw() const {
	if (isFinished_) {
		return;
	}
	plate_.Draw();
	disc_.Draw();
}
