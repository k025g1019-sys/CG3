#include "Game/UI/GameOverMenu.h"

#include "Engine/Core/WinApp.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector4.h"

using namespace Engine;

namespace {

// --- 画面を暗くする板 ---
const Vector4 kBackdropColor = { 0.0f, 0.0f, 0.0f, 0.45f };
// 板の奥行き（スプライトの描画範囲 0〜100 のうちの位置。深度は 0.99 になる）。
// 文字（カメラから 6 前後、深度 0.98 くらい）より奥、ステージ（カメラから 20 以上、深度 0.995 以上）より手前にして、
// ステージだけを暗くし、後から描く文字は隠さない
const float kBackdropDepth = 99.0f;

// --- 文字の既定値（Position はカメラからの相対位置。z がカメラからの距離）---
const Vector3 kGameOverPosition = { 0.0f, 0.5f, 6.0f };        // GAME OVER（上）
const float kGameOverScale = 0.5f;
const Vector3 kRetryPosition = { -1.2f, -0.6f, 6.0f };         // リトライ（左下）
const float kRetryScale = 0.3f;
const Vector3 kReturnToTitlePosition = { 1.2f, -0.6f, 6.0f };  // タイトルへ（右下）
const float kReturnToTitleScale = 0.3f;

const float kUnselectedRate = 0.4f;  // 選んでいない方の文字を暗くする倍率

} // namespace

#pragma region 初期化

void GameOverMenu::Initialize() {
	// 画面を暗くする板は 1x1 の白いテクスチャに色を掛けて使う
	const uint32_t whiteTexture = TextureManager::GetInstance()->GetWhiteTexture();
	const Vector2 screenSize = { static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight) };
	backdrop_.Initialize(whiteTexture, screenSize);
	backdrop_.GetTransform().translate = { 0.0f, 0.0f, kBackdropDepth };
	backdrop_.SetColor(kBackdropColor);

	// 文字（揺れなし。ImGui で揺らすこともできる）
	gameOverText_.Initialize("resources/textObjects/gameOver.obj", "GameOverText", kGameOverPosition, kGameOverScale, 0.0f);
	retryText_.Initialize("resources/textObjects/reTry.obj", "RetryText", kRetryPosition, kRetryScale, 0.0f);
	returnToTitleText_.Initialize("resources/textObjects/returnToTitle.obj", "ReturnToTitleText", kReturnToTitlePosition, kReturnToTitleScale, 0.0f);

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

void GameOverMenu::Update(float screenWidth, float screenHeight, const Vector3& cameraPosition) {
	// 閉じている間は何もしない（描かないので更新もしない）
	if (!isOpen_) {
		return;
	}

	// 表示した直後は入力を受け付けない（やられる直前から押していたキーで決まってしまわないように）
	timer_ += kFrameTime;
	if (timer_ >= kInputDelay && !isDecided_) {
		UpdateInput();
	}

	UpdateDisplay(screenWidth, screenHeight, cameraPosition);
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

void GameOverMenu::UpdateDisplay(float screenWidth, float screenHeight, const Vector3& cameraPosition) {
	// 画面を暗くする板は、描画先の縦横比が 1280x720 と違っても全体を覆う大きさにする（Fade と同じ考え方）
	const float scaleX = screenWidth / static_cast<float>(WinApp::kClientWidth);
	const float scaleY = screenHeight / static_cast<float>(WinApp::kClientHeight);
	const float sizeScale = (scaleX < scaleY) ? scaleX : scaleY;
	if (sizeScale > 0.0f) {
		backdrop_.SetSize({ screenWidth / sizeScale, screenHeight / sizeScale });
	}
	backdrop_.Update();

	// 選んでいる方を元の色、選んでいない方を暗くする
	const bool isRetrySelected = (choice_ == Choice::kRetry);
	retryText_.SetBrightness(isRetrySelected ? 1.0f : kUnselectedRate);
	returnToTitleText_.SetBrightness(isRetrySelected ? kUnselectedRate : 1.0f);

	// 文字はカメラの前に置く（カメラは回転しないので、位置を足すだけで画面に貼り付いて見える）
	gameOverText_.SetOrigin(cameraPosition);
	retryText_.SetOrigin(cameraPosition);
	returnToTitleText_.SetOrigin(cameraPosition);
	gameOverText_.Update();
	retryText_.Update();
	returnToTitleText_.Update();
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

	// 暗くする板の後に文字を描く（板は奥にずらしてあるので、文字は隠れない）
	backdrop_.Draw();
	gameOverText_.Draw();
	retryText_.Draw();
	returnToTitleText_.Draw();
}

#pragma endregion
