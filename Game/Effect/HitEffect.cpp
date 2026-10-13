#include "Game/Effect/HitEffect.h"

#include "Game/Util/Easing.h"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Engine;

void HitEffect::Initialize(const Vector3& position) {
	Initialize(position, { 1.0f, 0.95f, 0.6f, 1.0f });
}

void HitEffect::Initialize(const Vector3& position, const Vector4& color) {
	position_ = position;
	color_ = color;
	counter_ = 0.0f;
	isFinished_ = false;

	// 円盤（ライティングなしで色をそのまま出す）
	disc_.Initialize(Primitive::kSphere, LightingMode::kNone);
	disc_.GetTransform().translate = position_;
	disc_.GetTransform().scale = { kStartSize, kStartSize, kThickness };

	// 火花：斜め 4 方向（45 度から 90 度ずつ）へ飛ばす
	const float quarter = std::numbers::pi_v<float> * 0.5f;
	for (uint32_t i = 0; i < kNumSparks; ++i) {
		const float angle = quarter * 0.5f + quarter * static_cast<float>(i);
		sparkVelocities_[i] = { std::cos(angle) * kSparkSpeed, std::sin(angle) * kSparkSpeed, 0.0f };

		sparks_[i].Initialize(Primitive::kCube, LightingMode::kNone);
		sparks_[i].GetTransform().translate = position_;
		sparks_[i].GetTransform().scale = { kSparkSize, kSparkSize, kSparkSize };
	}
}

void HitEffect::Update() {
	if (isFinished_) {
		return;
	}

	// 経過時間を進める。広がって薄くなり終わったら終了
	counter_ += kFrameTime;
	const float total = kExpandDuration + kFadeDuration;
	if (counter_ >= total) {
		counter_ = total;
		isFinished_ = true;
	}

	// 円盤：最初が速く広がり、その後は大きさを保ったまま薄くなる
	const float expandT = std::clamp(counter_ / kExpandDuration, 0.0f, 1.0f);
	const float size = kStartSize + (kEndSize - kStartSize) * EaseOutQuad(expandT);
	const float fadeT = std::clamp((counter_ - kExpandDuration) / kFadeDuration, 0.0f, 1.0f);
	const Vector4 color = { color_.x, color_.y, color_.z, kMaxAlpha * (1.0f - fadeT) };

	disc_.GetTransform().scale = { size, size, kThickness };
	disc_.SetColor(color);
	disc_.Update();

	// 火花：減速しながら外へ飛び、回りながら円盤と同じ速さで薄くなる
	for (uint32_t i = 0; i < kNumSparks; ++i) {
		sparkVelocities_[i] = sparkVelocities_[i] * (1.0f - kSparkAttenuation);
		Transform3D& spark = sparks_[i].GetTransform();
		spark.translate += sparkVelocities_[i];
		spark.rotate.z += kSparkSpinSpeed;
		sparks_[i].SetColor(color);
		sparks_[i].Update();
	}
}

void HitEffect::Draw() const {
	if (isFinished_) {
		return;
	}
	disc_.Draw();
	for (const Object3D& spark : sparks_) {
		spark.Draw();
	}
}
