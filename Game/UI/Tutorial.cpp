#include "Game/UI/Tutorial.h"

#include "Game/Object/Player.h"

#include <cassert>

using namespace Engine;

namespace {

// --- 文字の既定値（Position はカメラからの相対位置。z がカメラからの距離）---
const Vector3 kTextPosition = { -1.1f, 0.75f, 6.0f };  // 画面の左上（ライフのゲージの下。右上に浮いているボスと重ならないように）
const float kTextScale = 0.14f;

} // namespace

#pragma region 初期化

void Tutorial::Initialize(const Player* player) {
	assert(player);
	player_ = player;

	// 文字（揺れなし。ImGui で揺らすこともできる）
	moveText_.Initialize("resources/textObjects/tutorialMove.obj", "TutorialMove", kTextPosition, kTextScale, 0.0f);
	attackText_.Initialize("resources/textObjects/tutorialAttack.obj", "TutorialAttack", kTextPosition, kTextScale, 0.0f);
	curveText_.Initialize("resources/textObjects/tutorialCurve.obj", "TutorialCurve", kTextPosition, kTextScale, 0.0f);

	wasAttacking_ = false;
	wasFlying_ = false;
	StartStep(Step::kMove);
}

#pragma endregion

#pragma region 更新

void Tutorial::Update(const Vector3& cameraPosition) {
	CheckActions();

	// ブロックの行動をすべてしたら、少し待って次のブロックへ
	if (IsStepCleared()) {
		clearTimer_ += kFrameTime;
		if (clearTimer_ >= kNextStepDelay) {
			if (step_ == Step::kMove) {
				StartStep(Step::kAttack);
			} else if (step_ == Step::kAttack) {
				StartStep(Step::kCurve);
			}
		}
	}

	// 今のブロックの文字だけをカメラの前に置く（描くのも今のブロックだけ）
	TextObjects& text = GetCurrentText();
	text.SetOrigin(cameraPosition);
	text.Update();
}

void Tutorial::CheckActions() {
	// 移動とジャンプは速度で見る（左右それぞれへ動いたか、床から上へ跳んだか）
	const Vector3& velocity = player_->GetVelocity();
	if (velocity.x < -kMoveSpeedThreshold) {
		hasMovedLeft_ = true;
	}
	if (velocity.x > kMoveSpeedThreshold) {
		hasMovedRight_ = true;
	}
	if (!player_->IsGrounded() && velocity.y > 0.0f) {
		hasJumped_ = true;
	}

	// 攻撃は、出し始めた瞬間で見る（前のブロックの間に出して、まだ続いている攻撃や飛んでいるブーメランは数えない）
	const bool isAttacking = player_->IsAttacking();
	const bool isFlying = player_->GetBoomerang().IsFlying();
	if (isAttacking && !wasAttacking_) {
		hasMeleeAttacked_ = true;
	}
	if (isFlying && !wasFlying_) {
		hasThrown_ = true;
	}
	wasAttacking_ = isAttacking;
	wasFlying_ = isFlying;
}

void Tutorial::StartStep(Step step) {
	// 次のブロックへ。した行動は数え直す（前のブロックの間にした行動で、次のブロックがすぐ終わらないように）
	step_ = step;
	clearTimer_ = 0.0f;
	hasMovedLeft_ = false;
	hasMovedRight_ = false;
	hasJumped_ = false;
	hasMeleeAttacked_ = false;
	hasThrown_ = false;
}

bool Tutorial::IsStepCleared() const {
	switch (step_) {
	case Step::kMove:
		return hasMovedLeft_ && hasMovedRight_ && hasJumped_;
	case Step::kAttack:
		return hasMeleeAttacked_ && hasThrown_;
	case Step::kCurve:
	default:
		return false;
	}
}

TextObjects& Tutorial::GetCurrentText() {
	switch (step_) {
	case Step::kMove:
		return moveText_;
	case Step::kAttack:
		return attackText_;
	case Step::kCurve:
	default:
		return curveText_;
	}
}

const TextObjects& Tutorial::GetCurrentText() const {
	switch (step_) {
	case Step::kMove:
		return moveText_;
	case Step::kAttack:
		return attackText_;
	case Step::kCurve:
	default:
		return curveText_;
	}
}

const char* Tutorial::GetStepName() const {
	switch (step_) {
	case Step::kMove:
		return "move";
	case Step::kAttack:
		return "attack";
	case Step::kCurve:
	default:
		return "curve";
	}
}

#pragma endregion

#pragma region 描画

void Tutorial::Draw() const {
	GetCurrentText().Draw();
}

#pragma endregion
