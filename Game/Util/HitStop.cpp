#include "Game/Util/HitStop.h"

#include <cmath>

using namespace Engine;

void HitStop::Start(float duration, float shakeAmplitude) {
	timer_ = duration;
	duration_ = duration;
	shakeAmplitude_ = shakeAmplitude;
}

void HitStop::Update() {
	if (timer_ <= 0.0f) {
		return;
	}
	timer_ -= kFrameTime;
	if (timer_ < 0.0f) {
		timer_ = 0.0f;
	}
}

void HitStop::Stop() {
	timer_ = 0.0f;
}

Vector3 HitStop::GetShakeOffset() const {
	if (!IsActive() || duration_ <= 0.0f) {
		return { 0.0f, 0.0f, 0.0f };
	}

	// 経過時間で左右に振り、残り時間の割合で揺れ幅を小さくしていく
	const float elapsed = duration_ - timer_;
	const float rate = timer_ / duration_;
	return { std::sin(elapsed * kShakeSpeed) * shakeAmplitude_ * rate, 0.0f, 0.0f };
}
