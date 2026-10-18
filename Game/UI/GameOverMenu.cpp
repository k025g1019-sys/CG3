#include "Game/UI/GameOverMenu.h"

#include "Engine/Core/WinApp.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector4.h"

using namespace Engine;

namespace {

// --- 配置（1280x720 基準の px。左上の角）---
const Vector2 kBannerPosition = { 380.0f, 190.0f };  // GAME OVER の板
const Vector2 kBannerSize = { 520.0f, 90.0f };
const Vector2 kRetryPosition = { 360.0f, 400.0f };   // リトライの板（左）
const Vector2 kTitlePosition = { 680.0f, 400.0f };   // タイトルへの板（右）
const Vector2 kPanelSize = { 240.0f, 80.0f };
const float kCursorBorder = 6.0f;                    // 枠を板より広げる幅

// --- 色 ---
const Vector4 kBackdropColor = { 0.0f, 0.0f, 0.0f, 0.45f };  // 画面を暗くする板
const Vector4 kBannerColor = { 0.7f, 0.12f, 0.12f, 0.95f };  // GAME OVER の板
const Vector4 kCursorColor = { 1.0f, 1.0f, 1.0f, 1.0f };     // 選んでいる方の枠
const Vector4 kRetryColor = { 0.3f, 0.8f, 0.4f, 1.0f };      // リトライの板
const Vector4 kTitleColor = { 0.4f, 0.55f, 0.9f, 1.0f };     // タイトルへの板
const float kUnselectedRate = 0.4f;                          // 選んでいない方の板を暗くする倍率

// 選んでいない方の色（暗くする。不透明のまま）
Vector4 MakeUnselectedColor(const Vector4& color) {
	return { color.x * kUnselectedRate, color.y * kUnselectedRate, color.z * kUnselectedRate, color.w };
}

} // namespace

#pragma region 初期化

void GameOverMenu::Initialize() {
	// どのスプライトも 1x1 の白いテクスチャに色を掛けて使う（画像ができたら差し替える）
	const uint32_t whiteTexture = TextureManager::GetInstance()->GetWhiteTexture();
	const Vector2 screenSize = { static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight) };

	backdrop_.Initialize(whiteTexture, screenSize);
	backdrop_.SetColor(kBackdropColor);

	banner_.Initialize(whiteTexture, kBannerSize);
	banner_.GetTransform().translate = { kBannerPosition.x, kBannerPosition.y, 0.0f };
	banner_.SetColor(kBannerColor);

	cursor_.Initialize(whiteTexture, { kPanelSize.x + kCursorBorder * 2.0f, kPanelSize.y + kCursorBorder * 2.0f });
	cursor_.SetColor(kCursorColor);

	retryPanel_.Initialize(whiteTexture, kPanelSize);
	retryPanel_.GetTransform().translate = { kRetryPosition.x, kRetryPosition.y, 0.0f };

	titlePanel_.Initialize(whiteTexture, kPanelSize);
	titlePanel_.GetTransform().translate = { kTitlePosition.x, kTitlePosition.y, 0.0f };

	isOpen_ = false;
	isDecided_ = false;
	choice_ = Choice::kRetry;
	timer_ = 0.0f;
}

#pragma endregion

#pragma region 更新

void GameOverMenu::Open() {
	isOpen_ = true;
	isDecided_ = false;
	choice_ = Choice::kRetry;
	timer_ = 0.0f;
}

void GameOverMenu::Close() {
	isOpen_ = false;
}

void GameOverMenu::Update(float screenWidth, float screenHeight) {
	// 閉じている間は何もしない（描かないのでスプライトも更新しない）
	if (!isOpen_) {
		return;
	}

	// 表示した直後は入力を受け付けない（やられる直前から押していたキーで決まってしまわないように）
	timer_ += kFrameTime;
	if (timer_ >= kInputDelay && !isDecided_) {
		UpdateInput();
	}

	UpdateSprites(screenWidth, screenHeight);
}

void GameOverMenu::UpdateInput() {
	Input* input = Input::GetInstance();

	// 左右で選ぶ（左がリトライ、右がタイトルへ）
	if (input->IsTrigger(DIK_A) || input->IsTrigger(DIK_LEFT)) {
		choice_ = Choice::kRetry;
	}
	if (input->IsTrigger(DIK_D) || input->IsTrigger(DIK_RIGHT)) {
		choice_ = Choice::kTitle;
	}

	// Space / Enter で決める
	if (input->IsTrigger(DIK_SPACE) || input->IsTrigger(DIK_RETURN)) {
		isDecided_ = true;
	}
}

void GameOverMenu::UpdateSprites(float screenWidth, float screenHeight) {
	// 画面を暗くする板は、描画先の縦横比が 1280x720 と違っても全体を覆う大きさにする（Fade と同じ考え方）
	const float scaleX = screenWidth / static_cast<float>(WinApp::kClientWidth);
	const float scaleY = screenHeight / static_cast<float>(WinApp::kClientHeight);
	const float sizeScale = (scaleX < scaleY) ? scaleX : scaleY;
	if (sizeScale > 0.0f) {
		backdrop_.SetSize({ screenWidth / sizeScale, screenHeight / sizeScale });
	}

	// 選んでいる方を明るく、選んでいない方を暗くし、選んでいる方の後ろに枠を置く
	const bool isRetrySelected = (choice_ == Choice::kRetry);
	retryPanel_.SetColor(isRetrySelected ? kRetryColor : MakeUnselectedColor(kRetryColor));
	titlePanel_.SetColor(isRetrySelected ? MakeUnselectedColor(kTitleColor) : kTitleColor);

	const Vector2& selectedPosition = isRetrySelected ? kRetryPosition : kTitlePosition;
	cursor_.GetTransform().translate = { selectedPosition.x - kCursorBorder, selectedPosition.y - kCursorBorder, 0.0f };

	backdrop_.Update();
	banner_.Update();
	cursor_.Update();
	retryPanel_.Update();
	titlePanel_.Update();
}

const char* GameOverMenu::GetChoiceName() const {
	return (choice_ == Choice::kRetry) ? "retry" : "title";
}

#pragma endregion

#pragma region 描画

void GameOverMenu::Draw() const {
	if (!isOpen_) {
		return;
	}

	// 奥から順に重ねる
	backdrop_.Draw();
	banner_.Draw();
	cursor_.Draw();
	retryPanel_.Draw();
	titlePanel_.Draw();
}

#pragma endregion
