#include "Game/Effect/DeathParticles.h"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Engine;

void DeathParticles::Initialize(const Vector3& position) {
	// 全パーティクルを発生位置にそろえる
	for (Object3D& particle : particles_) {
		particle.Initialize(Primitive::kSphere);
		particle.GetTransform().translate = position;
		particle.GetTransform().scale = { kScale, kScale, kScale };
	}

	// 最初は不透明な白
	color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	counter_ = 0.0f;
	isFinished_ = false;
}

void DeathParticles::Update() {
	// 終わっていたら何もしない
	if (isFinished_) {
		return;
	}

	// 経過時間を進める。消える時間に達したら終わり
	counter_ += kFrameTime;
	if (counter_ >= kDuration) {
		counter_ = kDuration;
		isFinished_ = true;
	}

	// 1 つあたりの角度（360 度を均等に割る）
	const float angleUnit = 2.0f * std::numbers::pi_v<float> / static_cast<float>(kNumParticles);

	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// 右向きの速度を i 番目の角度ぶん回し、外側へ動かす
		const float angle = angleUnit * static_cast<float>(i);
		const Vector3 velocity = { kSpeed * std::cos(angle), kSpeed * std::sin(angle), 0.0f };
		particles_[i].GetTransform().translate += velocity;
	}

	// 経過に合わせてアルファを 1 → 0 へ下げて消していく
	color_.w = std::clamp(1.0f - counter_ / kDuration, 0.0f, 1.0f);
	for (Object3D& particle : particles_) {
		particle.SetColor(color_);
		particle.Update();
	}
}

void DeathParticles::Draw() const {
	// 終わった後は描かない
	if (isFinished_) {
		return;
	}
	for (const Object3D& particle : particles_) {
		particle.Draw();
	}
}
