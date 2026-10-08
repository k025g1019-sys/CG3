#include "Game/UI/Gauge.h"

#include "Engine/Graphics/TextureManager.h"
#include "Engine/Math/Matrix4x4.h"

#include <algorithm>

using namespace Engine;

namespace {

const Vector4 kBackgroundColor = { 0.08f, 0.08f, 0.1f, 0.85f };  // 背景の色
const Vector4 kDelayBarColor = { 1.0f, 1.0f, 1.0f, 0.9f };      // 遅れバーの色

} // namespace

void Gauge::Initialize(const Vector2& position, const Vector2& size, const Vector4& barColor) {
	position_ = position;
	size_ = size;
	delayRatio_ = 1.0f;

	// どのスプライトも 1x1 の白いテクスチャに色を掛けて使う
	const uint32_t whiteTexture = TextureManager::GetInstance()->GetWhiteTexture();

	// 背景はバーより kBorder ぶん広げて枠に見せる
	background_.Initialize(whiteTexture, { size.x + kBorder * 2.0f, size.y + kBorder * 2.0f });
	background_.GetTransform().translate = { position.x - kBorder, position.y - kBorder, 0.0f };
	background_.SetColor(kBackgroundColor);

	delayBar_.Initialize(whiteTexture, size);
	delayBar_.GetTransform().translate = { position.x, position.y, 0.0f };
	delayBar_.SetColor(kDelayBarColor);

	bar_.Initialize(whiteTexture, size);
	bar_.GetTransform().translate = { position.x, position.y, 0.0f };
	bar_.SetColor(barColor);
}

void Gauge::Update(float ratio) {
	ratio = std::clamp(ratio, 0.0f, 1.0f);

	// 遅れバーは減るときだけゆっくり追いかけ、増えるときはすぐ合わせる
	if (delayRatio_ > ratio) {
		delayRatio_ = Lerp(delayRatio_, ratio, kDelayFollowRate);
		if (delayRatio_ - ratio < 0.001f) {
			delayRatio_ = ratio;
		}
	} else {
		delayRatio_ = ratio;
	}

	// 残量は横幅で表す（左端を固定して右へ伸び縮みする）
	bar_.SetSize({ size_.x * ratio, size_.y });
	delayBar_.SetSize({ size_.x * delayRatio_, size_.y });

	background_.Update();
	delayBar_.Update();
	bar_.Update();
}

void Gauge::Draw() const {
	// 奥から順に重ねる
	background_.Draw();
	delayBar_.Draw();
	bar_.Draw();
}
