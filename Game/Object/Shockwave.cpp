#include "Game/Object/Shockwave.h"

#include "Game/Stage/Stage.h"

#include <cassert>

using namespace Engine;

void Shockwave::Initialize(const Vector3& position, float direction, const Stage* stage) {
	assert(stage);
	stage_ = stage;
	direction_ = (direction >= 0.0f) ? 1.0f : -1.0f;

	// 床の上を滑る高さに置く
	position_ = { position.x, stage_->GetGroundY() + kRadius, 0.0f };

	model_.Initialize(Primitive::kCube);
	model_.SetColor({ 0.9f, 0.7f, 0.4f, 1.0f });
	model_.GetTransform().scale = { kRadius * 2.0f, kRadius * 1.6f, kRadius * 2.0f };
	model_.GetTransform().translate = position_;
	model_.Update();
}

void Shockwave::Update() {
	position_.x += direction_ * kSpeed;

	// ステージの端に届いたら消える
	if (position_.x <= stage_->GetLeftEdge() || position_.x >= stage_->GetRightEdge()) {
		Kill();
	}

	model_.GetTransform().translate = position_;
	model_.Update();
}

void Shockwave::Draw() const {
	model_.Draw();
}
