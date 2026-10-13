#include "Game/UI/Fade.h"

#include "Engine/Core/WinApp.h"
#include "Engine/Graphics/TextureManager.h"

#include <algorithm>

using namespace Engine;

void Fade::Initialize() {
	// 1x1 の白いテクスチャを黒く塗り、画面全体に引き伸ばす（スプライトの大きさは 1280x720 基準の px）
	const uint32_t whiteTexture = TextureManager::GetInstance()->GetWhiteTexture();
	const Vector2 screenSize = { static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight) };
	sprite_.Initialize(whiteTexture, screenSize);
	sprite_.SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

	status_ = Status::kNone;
	duration_ = 0.0f;
	counter_ = 0.0f;
}

void Fade::Update(float screenWidth, float screenHeight) {
	// フェードなしの間は何もしない
	if (status_ == Status::kNone) {
		return;
	}

	// スプライトは縦横の小さい方の倍率で等比に拡大されるので、描画先が横長（縦長）でも端が空かないよう、
	// その倍率で割った大きさにして全体を覆う（全画面にすると右端が途切れていた対策）
	const float scaleX = screenWidth / static_cast<float>(WinApp::kClientWidth);
	const float scaleY = screenHeight / static_cast<float>(WinApp::kClientHeight);
	const float sizeScale = (scaleX < scaleY) ? scaleX : scaleY;
	if (sizeScale > 0.0f) {
		sprite_.SetSize({ screenWidth / sizeScale, screenHeight / sizeScale });
	}

	// 経過時間を進める（かける時間で頭打ち）
	counter_ += kFrameTime;
	if (counter_ >= duration_) {
		counter_ = duration_;
	}

	// フェードインは黒 → 透明（アルファ 1 → 0）、フェードアウトは透明 → 黒（アルファ 0 → 1）
	const float t = (duration_ > 0.0f) ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f;
	const float alpha = (status_ == Status::kFadeIn) ? (1.0f - t) : t;
	sprite_.SetColor({ 0.0f, 0.0f, 0.0f, alpha });
	sprite_.Update();
}

void Fade::Draw() const {
	// フェードなし（透明）の間は描かない
	if (status_ == Status::kNone) {
		return;
	}
	sprite_.Draw();
}

void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::Stop() {
	status_ = Status::kNone;
}

bool Fade::IsFinished() const {
	if (status_ == Status::kNone) {
		return true;
	}
	return counter_ >= duration_;
}
