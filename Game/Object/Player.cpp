#include "Game/Object/Player.h"

#include "Engine/Input/Input.h"
#include "Engine/Math/Curve.h"
#include "Engine/Rendering/DebugDraw.h"
#include "Game/Stage/Stage.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

const Vector4 kBodyColor = { 1.0f, 0.4f, 0.3f, 1.0f };     // 通常の色
const Vector4 kDamagedColor = { 1.0f, 0.85f, 0.8f, 1.0f };  // 無敵中の点滅色

} // namespace

#pragma region 初期化

void Player::Initialize(const Stage* stage) {
	assert(stage);
	stage_ = stage;

	// 仮モデル（組み込みの立方体）。モデルができたら model_.Initialize("resources/player.obj") に差し替える
	model_.Initialize(Primitive::kCube);
	model_.SetColor(kBodyColor);

	// 攻撃判定の見た目（半透明の黄色い球）
	attackMark_.Initialize(Primitive::kSphere);
	attackMark_.SetColor({ 1.0f, 0.9f, 0.2f, 0.35f });

	// ステージの左寄り、床の上に立たせる
	model_.GetTransform().translate = { stage_->GetLeftEdge() + 6.0f, stage_->GetGroundY() + kHeight * 0.5f, 0.0f };
	velocity_ = { 0.0f, 0.0f, 0.0f };
	lrDirection_ = LRDirection::kRight;
	isGrounded_ = true;
	isGliding_ = false;
	coyoteTimer_ = 0.0f;

	attackType_ = AttackType::kNone;
	attackTimer_ = 0.0f;
	attackHasHit_ = false;

	isAiming_ = false;
	curveOffset_ = 0.0f;
	boomerang_.Initialize();

	invincibleTimer_ = 0.0f;
	life_ = kMaxLife;
}

#pragma endregion

#pragma region 更新

void Player::Update() {
	UpdateMove();
	UpdateJump();
	UpdatePhysics();
	UpdateCollision();
	UpdateAttack();
	UpdateAim();
	UpdateDirection();
	UpdateInvincible();

	boomerang_.Update(GetPosition());
	model_.Update();
	UpdateAttackMark();
}

void Player::UpdateMove() {
	Input* input = Input::GetInstance();
	const bool right = input->IsPress(DIK_D);
	const bool left = input->IsPress(DIK_A);

	// 同時押しは相殺し、どちらか一方のときだけ動く
	if (right != left) {
		const float direction = right ? 1.0f : -1.0f;

		// 逆方向へ入力したら急ブレーキ
		if (velocity_.x * direction < 0.0f) {
			velocity_.x *= (1.0f - kBrakeAttenuation);
		}
		velocity_.x += direction * kAcceleration;
		lrDirection_ = right ? LRDirection::kRight : LRDirection::kLeft;
	} else {
		// 入力が無ければ減速
		velocity_.x *= (1.0f - kAttenuation);
	}

	velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
}

void Player::UpdateJump() {
	isGliding_ = false;

	// 床を離れてからの時間（コヨーテタイム用）
	if (isGrounded_) {
		coyoteTimer_ = 0.0f;
	} else {
		coyoteTimer_ += kFrameTime;
	}

	// ジャンプ開始：床の上か、床を離れた直後なら受け付ける
	if (IsJumpTrigger() && (isGrounded_ || coyoteTimer_ < kCoyoteTime)) {
		velocity_.y = kJumpSpeed;
		isGrounded_ = false;
		coyoteTimer_ = kCoyoteTime;  // 猶予を使い切る（空中でもう一度跳べないようにする）
		return;
	}

	if (isGrounded_) {
		return;
	}

	if (velocity_.y > 0.0f) {
		// 上昇中にキーを離していたら上昇を弱める（押した長さでジャンプの高さが変わる）
		if (!IsJumpPress()) {
			velocity_.y *= kJumpCutRate;
		}
	} else {
		// 落下中にキーを押していればゆっくり降下
		if (IsJumpPress()) {
			isGliding_ = true;
		}
	}
}

void Player::UpdatePhysics() {
	// 重力。ゆっくり降下中は落下速度の上限を下げる
	velocity_.y -= kGravity;
	const float fallLimit = isGliding_ ? kGlideFallSpeed : kMaxFallSpeed;
	if (velocity_.y < -fallLimit) {
		velocity_.y = -fallLimit;
	}

	model_.GetTransform().translate += velocity_;
}

void Player::UpdateCollision() {
	Vector3& position = model_.GetTransform().translate;

	// 床：下端が床に届いていて落下中なら、床の上に押し戻して着地
	const float standY = stage_->GetGroundY() + kHeight * 0.5f;
	if (position.y <= standY && velocity_.y <= 0.0f) {
		position.y = standY;
		velocity_.y = 0.0f;
		isGrounded_ = true;
	} else {
		isGrounded_ = false;
	}

	// 左右の端：はみ出したら止める
	const float minX = stage_->GetLeftEdge() + kWidth * 0.5f;
	const float maxX = stage_->GetRightEdge() - kWidth * 0.5f;
	if (position.x < minX) {
		position.x = minX;
		velocity_.x = 0.0f;
	} else if (position.x > maxX) {
		position.x = maxX;
		velocity_.x = 0.0f;
	}

	// ゲームは Z=0 の平面で進む
	position.z = 0.0f;
}

void Player::UpdateAttack() {
	Input* input = Input::GetInstance();

	// 持続時間を減らし、終わったら解除。空中攻撃は着地でも終わる
	if (IsAttacking()) {
		attackTimer_ -= kFrameTime;
		const bool landedDuringAirAttack = (attackType_ == AttackType::kAir) && isGrounded_;
		if (attackTimer_ <= 0.0f || landedDuringAirAttack) {
			attackType_ = AttackType::kNone;
			attackTimer_ = 0.0f;
		}
	}

	// 左クリックで開始。攻撃中と、ブーメランが飛んでいる間は出せない
	if (input->IsMouseTrigger(kMouseLeft) && !IsAttacking() && !boomerang_.IsFlying()) {
		if (isGrounded_) {
			attackType_ = AttackType::kGround;
			attackTimer_ = kGroundAttackDuration;
		} else {
			attackType_ = AttackType::kAir;
			attackTimer_ = kAirAttackDuration;
		}
		attackHasHit_ = false;
	}

#ifndef NDEBUG
	// 開発中は判定を線で描く
	if (IsAttacking()) {
		DebugDraw::DrawSphere(GetAttackSphere(), { 1.0f, 0.3f, 0.2f, 1.0f });
	}
	if (boomerang_.IsFlying()) {
		DebugDraw::DrawSphere(boomerang_.GetHitSphere(), { 1.0f, 0.8f, 0.2f, 1.0f });
	}
#endif
}

void Player::UpdateAim() {
	Input* input = Input::GetInstance();

	// 右ボタンを押した瞬間に狙い始める（ブーメランが戻ってくるまでは投げられない）
	if (!isAiming_ && input->IsMouseTrigger(kMouseRight) && !boomerang_.IsFlying()) {
		isAiming_ = true;
		curveOffset_ = 0.0f;  // 曲線は毎回直線から始める
	}
	if (!isAiming_) {
		return;
	}

	// 離したら投げる（離した時点のカーソル位置が目標）
	if (!input->IsMousePress(kMouseRight)) {
		boomerang_.Throw(GetThrowOrigin(), cursorWorldPosition_, curveOffset_);
		isAiming_ = false;
		curveOffset_ = 0.0f;
		return;
	}

	// ホイールで予測線を曲げる（奥へ回すと上、手前へ回すと下。上限あり）
	const float wheel = input->GetWheel();
	if (wheel != 0.0f) {
		curveOffset_ += (wheel / kWheelNotch) * kCurveStepPerNotch;
		curveOffset_ = std::clamp(curveOffset_, -kMaxCurveOffset, kMaxCurveOffset);
	}

	// 予測線（ブーメランが飛ぶ曲線）と目標点の印
	const std::array<Vector3, 3> controlPoints = Boomerang::MakeControlPoints(GetThrowOrigin(), cursorWorldPosition_, curveOffset_);
	const Vector4 lineColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	DebugDraw::DrawCurve(Curve(controlPoints[0], controlPoints[1], controlPoints[2]), lineColor, false);
	DebugDraw::DrawSphere({ cursorWorldPosition_, 0.2f }, lineColor);
}

void Player::UpdateDirection() {
	// 狙っている間はカーソルの方を向く
	if (isAiming_) {
		lrDirection_ = (cursorWorldPosition_.x >= GetPosition().x) ? LRDirection::kRight : LRDirection::kLeft;
	}

	// 向きをモデルの Y 回転で表す（右向き +90 度、左向き -90 度。仮モデルの立方体では見た目に差は出ない）
	const float halfPi = std::numbers::pi_v<float> * 0.5f;
	model_.GetTransform().rotate.y = (lrDirection_ == LRDirection::kRight) ? halfPi : -halfPi;
}

void Player::UpdateInvincible() {
	if (invincibleTimer_ <= 0.0f) {
		model_.SetColor(kBodyColor);
		return;
	}
	invincibleTimer_ -= kFrameTime;

	// 0.1 秒ごとに色を切り替えて点滅させる
	const bool blink = std::fmod(invincibleTimer_, 0.2f) < 0.1f;
	model_.SetColor(blink ? kDamagedColor : kBodyColor);
}

void Player::OnDamage(float knockbackDirectionX) {
	if (IsInvincible() || IsDead()) {
		return;
	}
	life_--;

	// ライフが尽きたら、その場で止まる（吹き飛ばない。やられ演出はシーンが進める）
	if (life_ <= 0) {
		life_ = 0;
		velocity_ = { 0.0f, 0.0f, 0.0f };
		attackType_ = AttackType::kNone;
		isAiming_ = false;
		return;
	}
	invincibleTimer_ = kInvincibleDuration;

	// 相手から離れる向きに吹き飛ぶ
	const float direction = (knockbackDirectionX >= 0.0f) ? 1.0f : -1.0f;
	velocity_ = { direction * kDamageKnockbackSpeed, kDamageKnockbackUpSpeed, 0.0f };
	isGrounded_ = false;
}

void Player::UpdateAttackMark() {
	if (!IsAttacking()) {
		return;
	}
	// 球の仮モデルは直径 1 なので、半径の 2 倍に拡大する
	const Sphere sphere = GetAttackSphere();
	Transform3D& transform = attackMark_.GetTransform();
	transform.translate = sphere.center;
	transform.scale = { sphere.radius * 2.0f, sphere.radius * 2.0f, sphere.radius * 2.0f };
	attackMark_.Update();
}

Vector3 Player::GetThrowOrigin() const {
	return GetPosition() + Vector3{ 0.0f, kThrowOffsetY, 0.0f };
}

Vector3 Player::CalcAttackCenter() const {
	if (attackType_ == AttackType::kGround) {
		// 地上攻撃は向いている側の正面
		const float facing = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;
		return GetPosition() + Vector3{ facing * kGroundAttackOffsetX, 0.0f, 0.0f };
	}
	// 空中攻撃は自分を中心にする
	return GetPosition();
}

Sphere Player::GetAttackSphere() const {
	const float radius = (attackType_ == AttackType::kAir) ? kAirAttackRadius : kGroundAttackRadius;
	return { CalcAttackCenter(), radius };
}

bool Player::IsJumpTrigger() const {
	Input* input = Input::GetInstance();
	return input->IsTrigger(DIK_W) || input->IsTrigger(DIK_SPACE);
}

bool Player::IsJumpPress() const {
	Input* input = Input::GetInstance();
	return input->IsPress(DIK_W) || input->IsPress(DIK_SPACE);
}

#pragma endregion

#pragma region 描画

void Player::Draw() const {
	model_.Draw();
	boomerang_.Draw();
}

void Player::DrawEffect() const {
	if (IsAttacking()) {
		attackMark_.Draw();
	}
}

#ifdef USE_IMGUI
void Player::DrawImGui() {
	if (ImGui::TreeNode("Player")) {
		const Vector3& position = GetPosition();
		ImGui::Text("position: %.2f, %.2f", position.x, position.y);
		ImGui::Text("velocity: %.3f, %.3f", velocity_.x, velocity_.y);
		ImGui::Text("grounded: %s  gliding: %s  coyote: %.2f", isGrounded_ ? "true" : "false", isGliding_ ? "true" : "false", coyoteTimer_);
		ImGui::Text("direction: %s  life: %d / %d  invincible: %.2f", (lrDirection_ == LRDirection::kRight) ? "right" : "left", life_, kMaxLife, invincibleTimer_);

		const char* attackName = "none";
		if (attackType_ == AttackType::kGround) {
			attackName = "ground";
		} else if (attackType_ == AttackType::kAir) {
			attackName = "air";
		}
		ImGui::Text("attack: %s (%.2f s, hit: %s)  aiming: %s  curve: %.2f", attackName, attackTimer_, attackHasHit_ ? "true" : "false", isAiming_ ? "true" : "false", curveOffset_);
		ImGui::Text("cursor: %.2f, %.2f", cursorWorldPosition_.x, cursorWorldPosition_.y);
		ImGui::Separator();

		if (ImGui::TreeNode("Move / Jump")) {
			ImGui::DragFloat("acceleration", &kAcceleration, 0.001f, 0.0f, 1.0f, "%.4f");
			ImGui::DragFloat("attenuation", &kAttenuation, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("brakeAttenuation", &kBrakeAttenuation, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("limitRunSpeed", &kLimitRunSpeed, 0.005f, 0.0f, 2.0f);
			ImGui::DragFloat("jumpSpeed", &kJumpSpeed, 0.005f, 0.0f, 2.0f);
			ImGui::DragFloat("jumpCutRate", &kJumpCutRate, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("gravity", &kGravity, 0.0005f, 0.0f, 0.1f, "%.4f");
			ImGui::DragFloat("maxFallSpeed", &kMaxFallSpeed, 0.005f, 0.0f, 2.0f);
			ImGui::DragFloat("glideFallSpeed", &kGlideFallSpeed, 0.005f, 0.0f, 1.0f);
			ImGui::DragFloat("coyoteTime", &kCoyoteTime, 0.01f, 0.0f, 1.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Attack / Aim / Damage")) {
			ImGui::DragFloat("groundAttackDuration", &kGroundAttackDuration, 0.01f, 0.0f, 3.0f);
			ImGui::DragFloat("airAttackDuration", &kAirAttackDuration, 0.01f, 0.0f, 3.0f);
			ImGui::DragFloat("groundAttackRadius", &kGroundAttackRadius, 0.01f, 0.1f, 5.0f);
			ImGui::DragFloat("groundAttackOffsetX", &kGroundAttackOffsetX, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("airAttackRadius", &kAirAttackRadius, 0.01f, 0.1f, 5.0f);
			ImGui::DragFloat("curveStepPerNotch", &kCurveStepPerNotch, 0.05f, 0.0f, 5.0f);
			ImGui::DragFloat("maxCurveOffset", &kMaxCurveOffset, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("throwOffsetY", &kThrowOffsetY, 0.01f, -2.0f, 2.0f);
			ImGui::DragFloat("invincibleDuration", &kInvincibleDuration, 0.05f, 0.0f, 5.0f);
			ImGui::DragFloat("damageKnockbackSpeed", &kDamageKnockbackSpeed, 0.01f, 0.0f, 2.0f);
			ImGui::DragFloat("damageKnockbackUpSpeed", &kDamageKnockbackUpSpeed, 0.01f, 0.0f, 2.0f);
			ImGui::TreePop();
		}
		boomerang_.DrawImGui();
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
