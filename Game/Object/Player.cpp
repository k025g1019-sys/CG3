#include "Game/Object/Player.h"

#include "Engine/Input/Input.h"
#include "Engine/Math/Curve.h"
#include "Engine/Rendering/DebugDraw.h"
#include "Game/Sound/SoundEffects.h"
#include "Game/Stage/Stage.h"
#include "Game/Util/GlobalVariables.h"

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
const int32_t kPredictionDivision = 32;                     // 予測線を描くときに曲線を分ける数

} // namespace

#pragma region 調整値

void Player::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Player";
	globalVariables->CreateGroup(groupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(groupName, "MaxLife", kMaxLife);
	globalVariables->AddItem(groupName, "MeleeDamage", kMeleeDamage);
	globalVariables->AddItem(groupName, "Acceleration", kAcceleration);
	globalVariables->AddItem(groupName, "Attenuation", kAttenuation);
	globalVariables->AddItem(groupName, "BrakeAttenuation", kBrakeAttenuation);
	globalVariables->AddItem(groupName, "LimitRunSpeed", kLimitRunSpeed);
	globalVariables->AddItem(groupName, "JumpSpeed", kJumpSpeed);
	globalVariables->AddItem(groupName, "JumpCutRate", kJumpCutRate);
	globalVariables->AddItem(groupName, "Gravity", kGravity);
	globalVariables->AddItem(groupName, "MaxFallSpeed", kMaxFallSpeed);
	globalVariables->AddItem(groupName, "GlideFallSpeed", kGlideFallSpeed);
	globalVariables->AddItem(groupName, "CoyoteTime", kCoyoteTime);
	globalVariables->AddItem(groupName, "ApexSpeedThreshold", kApexSpeedThreshold);
	globalVariables->AddItem(groupName, "ApexGravityRate", kApexGravityRate);
	globalVariables->AddItem(groupName, "JumpBufferTime", kJumpBufferTime);
	globalVariables->AddItem(groupName, "AttackJumpLockTime", kAttackJumpLockTime);
	globalVariables->AddItem(groupName, "GroundAttackDuration", kGroundAttackDuration);
	globalVariables->AddItem(groupName, "AirAttackDuration", kAirAttackDuration);
	globalVariables->AddItem(groupName, "GroundAttackRadius", kGroundAttackRadius);
	globalVariables->AddItem(groupName, "GroundAttackOffsetX", kGroundAttackOffsetX);
	globalVariables->AddItem(groupName, "AirAttackRadius", kAirAttackRadius);
	globalVariables->AddItem(groupName, "CurveStepPerNotch", kCurveStepPerNotch);
	globalVariables->AddItem(groupName, "WheelAwayCurvesUp", kWheelAwayCurvesUp);
	globalVariables->AddItem(groupName, "MaxCurveOffset", kMaxCurveOffset);
	globalVariables->AddItem(groupName, "ThrowOffsetY", kThrowOffsetY);
	globalVariables->AddItem(groupName, "CatchCooldown", kCatchCooldown);
	globalVariables->AddItem(groupName, "BurstCatchCooldown", kBurstCatchCooldown);
	globalVariables->AddItem(groupName, "BurstClickTime", kBurstClickTime);
	globalVariables->AddItem(groupName, "InvincibleDuration", kInvincibleDuration);
	globalVariables->AddItem(groupName, "DamageHitStopDuration", kDamageHitStopDuration);
	globalVariables->AddItem(groupName, "DamageShakeAmplitude", kDamageShakeAmplitude);
	globalVariables->AddItem(groupName, "DamageKnockbackSpeed", kDamageKnockbackSpeed);
	globalVariables->AddItem(groupName, "DamageKnockbackDuration", kDamageKnockbackDuration);
}

void Player::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Player";
	kMaxLife = globalVariables->GetIntValue(groupName, "MaxLife");
	kMeleeDamage = globalVariables->GetIntValue(groupName, "MeleeDamage");
	kAcceleration = globalVariables->GetFloatValue(groupName, "Acceleration");
	kAttenuation = globalVariables->GetFloatValue(groupName, "Attenuation");
	kBrakeAttenuation = globalVariables->GetFloatValue(groupName, "BrakeAttenuation");
	kLimitRunSpeed = globalVariables->GetFloatValue(groupName, "LimitRunSpeed");
	kJumpSpeed = globalVariables->GetFloatValue(groupName, "JumpSpeed");
	kJumpCutRate = globalVariables->GetFloatValue(groupName, "JumpCutRate");
	kGravity = globalVariables->GetFloatValue(groupName, "Gravity");
	kMaxFallSpeed = globalVariables->GetFloatValue(groupName, "MaxFallSpeed");
	kGlideFallSpeed = globalVariables->GetFloatValue(groupName, "GlideFallSpeed");
	kCoyoteTime = globalVariables->GetFloatValue(groupName, "CoyoteTime");
	kApexSpeedThreshold = globalVariables->GetFloatValue(groupName, "ApexSpeedThreshold");
	kApexGravityRate = globalVariables->GetFloatValue(groupName, "ApexGravityRate");
	kJumpBufferTime = globalVariables->GetFloatValue(groupName, "JumpBufferTime");
	kAttackJumpLockTime = globalVariables->GetFloatValue(groupName, "AttackJumpLockTime");
	kGroundAttackDuration = globalVariables->GetFloatValue(groupName, "GroundAttackDuration");
	kAirAttackDuration = globalVariables->GetFloatValue(groupName, "AirAttackDuration");
	kGroundAttackRadius = globalVariables->GetFloatValue(groupName, "GroundAttackRadius");
	kGroundAttackOffsetX = globalVariables->GetFloatValue(groupName, "GroundAttackOffsetX");
	kAirAttackRadius = globalVariables->GetFloatValue(groupName, "AirAttackRadius");
	kCurveStepPerNotch = globalVariables->GetFloatValue(groupName, "CurveStepPerNotch");
	kWheelAwayCurvesUp = globalVariables->GetIntValue(groupName, "WheelAwayCurvesUp");
	kMaxCurveOffset = globalVariables->GetFloatValue(groupName, "MaxCurveOffset");
	kThrowOffsetY = globalVariables->GetFloatValue(groupName, "ThrowOffsetY");
	kCatchCooldown = globalVariables->GetFloatValue(groupName, "CatchCooldown");
	kBurstCatchCooldown = globalVariables->GetFloatValue(groupName, "BurstCatchCooldown");
	kBurstClickTime = globalVariables->GetFloatValue(groupName, "BurstClickTime");
	kInvincibleDuration = globalVariables->GetFloatValue(groupName, "InvincibleDuration");
	kDamageHitStopDuration = globalVariables->GetFloatValue(groupName, "DamageHitStopDuration");
	kDamageShakeAmplitude = globalVariables->GetFloatValue(groupName, "DamageShakeAmplitude");
	kDamageKnockbackSpeed = globalVariables->GetFloatValue(groupName, "DamageKnockbackSpeed");
	kDamageKnockbackDuration = globalVariables->GetFloatValue(groupName, "DamageKnockbackDuration");
}

#pragma endregion

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

	// ブーメランは地面より下へ飛ばない
	boomerang_.Initialize(stage_->GetGroundY());

	Reset();
}

void Player::Reset() {
	// ステージの左寄り、床の上に立たせる
	model_.GetTransform().translate = { stage_->GetLeftEdge() + 6.0f, stage_->GetGroundY() + kHeight * 0.5f, 0.0f };
	model_.SetColor(kBodyColor);
	velocity_ = { 0.0f, 0.0f, 0.0f };
	lrDirection_ = LRDirection::kRight;
	isGrounded_ = true;
	isGliding_ = false;
	coyoteTimer_ = 0.0f;
	jumpBufferTimer_ = 0.0f;

	attackType_ = AttackType::kNone;
	attackTimer_ = 0.0f;
	attackHasHit_ = false;
	attackJumpLockTimer_ = 0.0f;

	isAiming_ = false;
	curveOffset_ = 0.0f;
	boomerang_.Reset();
	meleeCooldownTimer_ = 0.0f;
	aimCooldownTimer_ = 0.0f;
	followPressTimer_ = 0.0f;

	invincibleTimer_ = 0.0f;
	life_ = kMaxLife;
	hitStop_.Stop();
	isKnockbackPending_ = false;
	knockbackTimer_ = 0.0f;
	isControllable_ = true;
}

#pragma endregion

#pragma region 更新

void Player::Update() {
	// ヒットストップ中は自分の動きを止める（移動も攻撃の時間も無敵時間も進めない）。
	// 投げてあるブーメランは飛び続け、見た目は毎フレーム更新する（被弾のときは揺らす）
	if (hitStop_.IsActive()) {
		hitStop_.Update();
		UpdateBoomerang();
		UpdateBodyModel();
		UpdateAttackMark();

		// 狙っている最中なら、止まっている間も予測線を出し続ける（消すと点滅したように見える）
		if (isAiming_) {
			DrawAimLine();
		}
		return;
	}

	UpdateKnockback();
	UpdateMove();
	UpdateJump();
	UpdatePhysics();
	UpdateCollision();
	UpdateAttack();
	UpdateAim();
	UpdateBoomerangControl();
	UpdateDirection();
	UpdateInvincible();

	UpdateBoomerang();
	UpdateBodyModel();
	UpdateAttackMark();
}

void Player::UpdateBoomerang() {
	// キャッチ後のクールタイムを減らす
	if (meleeCooldownTimer_ > 0.0f) {
		meleeCooldownTimer_ -= kFrameTime;
	}
	if (aimCooldownTimer_ > 0.0f) {
		aimCooldownTimer_ -= kFrameTime;
	}

	// 飛んでいたブーメランが手元に戻った瞬間から、少しの間は次の攻撃を出せない。
	// 大技を出した投げのときは、狙い直しだけ少し長く待つ（近接は同じ）
	const bool wasFlying = boomerang_.IsFlying();
	boomerang_.Update(GetPosition());
	if (wasFlying && !boomerang_.IsFlying()) {
		SoundEffects::Play(SoundEffects::Id::kCatch);
		meleeCooldownTimer_ = kCatchCooldown;
		aimCooldownTimer_ = boomerang_.IsBurstUsed() ? kBurstCatchCooldown : kCatchCooldown;
	}
}

void Player::UpdateBodyModel() {
	// 揺れは見た目だけに掛ける。位置はモデルの translate そのものなので、ずらして更新したらすぐ元に戻す
	Vector3& translate = model_.GetTransform().translate;
	const Vector3 position = translate;
	translate += hitStop_.GetShakeOffset();
	model_.Update();
	translate = position;
}

void Player::UpdateKnockback() {
	// ヒットストップが終わった最初のフレームで、ノックバックを始める
	if (isKnockbackPending_) {
		isKnockbackPending_ = false;
		knockbackTimer_ = kDamageKnockbackDuration;
	}
	if (!IsKnockback()) {
		return;
	}

	// 攻撃から離れる向きへ滑る。残り時間の割合を速さに掛けて、終わりにかけて遅くする（上には跳ねない）
	const float rate = (kDamageKnockbackDuration > 0.0f) ? (knockbackTimer_ / kDamageKnockbackDuration) : 0.0f;
	velocity_.x = knockbackDirection_ * kDamageKnockbackSpeed * rate;

	knockbackTimer_ -= kFrameTime;
	if (knockbackTimer_ < 0.0f) {
		knockbackTimer_ = 0.0f;
	}
}

void Player::UpdateMove() {
	// ノックバックの最中は、ノックバックの速度のまま動く（入力で加速も減速もしない）
	if (IsKnockback()) {
		return;
	}

	// 操作を受け付けない間は、入力が無いものとして扱う（減速して止まる）
	Input* input = Input::GetInstance();
	const bool right = isControllable_ && input->IsPress(DIK_D);
	const bool left = isControllable_ && input->IsPress(DIK_A);

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

	// 先行入力：押した瞬間を少しの間おぼえておき、跳べるようになった時点で跳ぶ
	if (CanControl() && IsJumpTrigger()) {
		jumpBufferTimer_ = kJumpBufferTime;
	} else if (jumpBufferTimer_ > 0.0f) {
		jumpBufferTimer_ -= kFrameTime;
	}

	// 地上攻撃を出した直後はジャンプできない
	if (attackJumpLockTimer_ > 0.0f) {
		attackJumpLockTimer_ -= kFrameTime;
	}

	// ジャンプ開始：床の上か床を離れた直後で、攻撃のロック中でなければ受け付ける
	const bool canJump = (isGrounded_ || coyoteTimer_ < kCoyoteTime) && attackJumpLockTimer_ <= 0.0f;
	if (jumpBufferTimer_ > 0.0f && canJump) {
		velocity_.y = kJumpSpeed;
		isGrounded_ = false;
		SoundEffects::Play(SoundEffects::Id::kJump);
		coyoteTimer_ = kCoyoteTime;  // 猶予を使い切る（空中でもう一度跳べないようにする）
		jumpBufferTimer_ = 0.0f;
		return;
	}

	if (isGrounded_) {
		return;
	}

	// 操作できない間は、跳んでいる途中の高さの調整もゆっくり降下もしない（そのまま着地する）
	if (!CanControl()) {
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
	// 重力。頂点付近（上下の速度が小さい間）は弱めて滞空感を出し、ゆっくり降下中は落下速度の上限を下げる
	float gravity = kGravity;
	if (!isGrounded_ && std::fabs(velocity_.y) < kApexSpeedThreshold) {
		gravity *= kApexGravityRate;
	}
	velocity_.y -= gravity;
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

	// 持続時間を減らし、終わったら解除。地上攻撃は床を離れると、空中攻撃は着地すると終わる
	if (IsAttacking()) {
		attackTimer_ -= kFrameTime;
		const bool leftGroundDuringGroundAttack = (attackType_ == AttackType::kGround) && !isGrounded_;
		const bool landedDuringAirAttack = (attackType_ == AttackType::kAir) && isGrounded_;
		if (attackTimer_ <= 0.0f || leftGroundDuringGroundAttack || landedDuringAirAttack) {
			attackType_ = AttackType::kNone;
			attackTimer_ = 0.0f;
		}
	}

	// 左クリックで開始（狙っている間でも出せる）。攻撃中・ブーメランが飛んでいる間・キャッチした直後は出せない
	if (input->IsMouseTrigger(kMouseLeft) && !IsAttacking() && CanStartMelee()) {
		if (isGrounded_) {
			attackType_ = AttackType::kGround;
			attackTimer_ = kGroundAttackDuration;
			attackJumpLockTimer_ = kAttackJumpLockTime;  // 出した直後は跳べない
		} else {
			attackType_ = AttackType::kAir;
			attackTimer_ = kAirAttackDuration;
		}
		attackHasHit_ = false;
		SoundEffects::Play(SoundEffects::Id::kSwing);
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

	// 右ボタンを押している間、狙える状態になったら狙い始める。
	// 近接攻撃の最中・ブーメランが飛んでいる間・キャッチした直後は狙えない（押したままなら、狙えるようになった時点で始まる）
	if (!isAiming_ && input->IsMousePress(kMouseRight) && !IsAttacking() && CanStartAim()) {
		isAiming_ = true;
		curveOffset_ = 0.0f;  // 曲線は毎回直線から始める
	}
	if (!isAiming_) {
		return;
	}

	// 離したら投げる（離した時点のカーソル位置が目標）。
	// 近接攻撃の最中とノックバックの最中は投げられないので、そのときに離したら投げずに狙いを取り消す
	if (!input->IsMousePress(kMouseRight)) {
		if (!IsAttacking() && CanControl()) {
			boomerang_.Throw(GetThrowOrigin(), cursorWorldPosition_, curveOffset_);
			SoundEffects::Play(SoundEffects::Id::kThrow);
		}
		isAiming_ = false;
		curveOffset_ = 0.0f;
		return;
	}

	// ホイールで予測線を曲げる（上限あり）。回す向きと曲がる向きの対応は調整値で選べる
	// （WheelAwayCurvesUp が 1 なら奥へ回すと上・手前へ回すと下、0 ならその逆）
	const float wheel = input->GetWheel();
	if (wheel != 0.0f) {
		const float direction = (kWheelAwayCurvesUp != 0) ? 1.0f : -1.0f;
		curveOffset_ += direction * (wheel / kWheelNotch) * kCurveStepPerNotch;
		curveOffset_ = std::clamp(curveOffset_, -kMaxCurveOffset, kMaxCurveOffset);
	}

	DrawAimLine();
}

void Player::DrawAimLine() const {
	// 予測線（ブーメランが飛ぶ曲線）。曲線を細かい線分でなぞり、地面に当たる所から先は飛ばないのでそこで止める
	const std::array<Vector3, 3> controlPoints = Boomerang::MakeControlPoints(GetThrowOrigin(), cursorWorldPosition_, curveOffset_);
	const Vector4 lineColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	const float minY = stage_->GetGroundY() + Boomerang::GetRadius();
	Vector3 previous = controlPoints[0];
	for (int32_t i = 1; i <= kPredictionDivision; ++i) {
		const float t = static_cast<float>(i) / static_cast<float>(kPredictionDivision);
		Vector3 point = Bezier(controlPoints[0], controlPoints[1], controlPoints[2], t);
		const bool hitsGround = point.y <= minY;
		if (hitsGround) {
			point.y = minY;
		}
		DebugDraw::DrawLine(previous, point, lineColor);
		previous = point;
		if (hitsGround) {
			break;
		}
	}

	// ブーメランが折り返す位置の印（地面に当たるならその位置、当たらなければカーソルの位置）
	DebugDraw::DrawSphere({ previous, 0.2f }, lineColor);
}

void Player::UpdateBoomerangControl() {
	Input* input = Input::GetInstance();

	// 追従中：押している間はカーソルを追わせる。離したら戻す（押してすぐ離したなら大技にする）
	if (boomerang_.IsFollowing()) {
		followPressTimer_ += kFrameTime;
		if (input->IsMousePress(kMouseRight)) {
			boomerang_.SetFollowTarget(cursorWorldPosition_);
			return;
		}
		if (followPressTimer_ <= kBurstClickTime) {
			boomerang_.StartBurst();
		} else {
			boomerang_.EndFollow();
		}
		return;
	}

	// 飛んでいる間に右ボタンを押したら、押した瞬間から追従を始める（追従をやめた後の戻りと、大技の後は始めない）
	if (CanControl() && input->IsMouseTrigger(kMouseRight) && boomerang_.CanStartFollow()) {
		boomerang_.StartFollow(cursorWorldPosition_);
		followPressTimer_ = 0.0f;
	}
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

bool Player::OnDamage(float knockbackDirectionX) {
	if (IsInvincible() || IsDead()) {
		return false;
	}

	// 開発用の無敵モードではライフを減らさない（ヒットストップと無敵の演出はそのまま）
	if (!isInvincibleMode_) {
		life_--;
	}

	// 一瞬止まって揺れる（ヒットストップ）。止まっている間は白っぽい色にしておく
	hitStop_.Start(kDamageHitStopDuration, kDamageShakeAmplitude);
	SoundEffects::Play(SoundEffects::Id::kDamage);
	model_.SetColor(kDamagedColor);

	// ライフが尽きたら、その場で止まる（やられ演出はシーンが進める）
	if (life_ <= 0) {
		life_ = 0;
		velocity_ = { 0.0f, 0.0f, 0.0f };
		attackType_ = AttackType::kNone;
		isAiming_ = false;
		return true;
	}

	// 無敵時間。ヒットストップ中は UpdateInvincible を呼ばないので、ノックバックが始まってから減り始める
	invincibleTimer_ = kInvincibleDuration;

	// ヒットストップが終わったら、続けて攻撃から離れる向きへノックバックする
	isKnockbackPending_ = true;
	knockbackDirection_ = (knockbackDirectionX >= 0.0f) ? 1.0f : -1.0f;
	jumpBufferTimer_ = 0.0f;  // ノックバック中に先行入力で跳ばないようにする
	return true;
}

void Player::SetControllable(bool isControllable) {
	isControllable_ = isControllable;

	// 受け付けなくなったら、まだ始まっていない行動は取り消す（先行入力のジャンプ、狙っている途中の投擲）
	if (!isControllable_) {
		jumpBufferTimer_ = 0.0f;
		isAiming_ = false;
		curveOffset_ = 0.0f;

		// 追従させていたブーメランは戻す
		boomerang_.EndFollow();
	}
}

void Player::UpdateModel() {
	// やられた瞬間の揺れは、止めて見せている間もここで進める
	hitStop_.Update();
	UpdateBodyModel();
	boomerang_.UpdateModel();
	UpdateAttackMark();
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

bool Player::CanStartMelee() const {
	return CanControl() && !boomerang_.IsFlying() && meleeCooldownTimer_ <= 0.0f;
}

bool Player::CanStartAim() const {
	return CanControl() && !boomerang_.IsFlying() && aimCooldownTimer_ <= 0.0f;
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
void Player::DrawWheelDirectionImGui() {
	// チェックを変えたら GlobalVariables の値を書き換える（調整値は毎フレームそこから読み直しているため、
	// ここで kWheelAwayCurvesUp だけを変えても次のフレームに戻ってしまう）
	bool isAwayUp = (kWheelAwayCurvesUp != 0);
	if (ImGui::Checkbox("wheel away = curve up  (off: wheel toward = curve up)", &isAwayUp)) {
		const int32_t value = isAwayUp ? 1 : 0;
		GlobalVariables::GetInstance()->SetValue("Player", "WheelAwayCurvesUp", value);
	}
}

void Player::DrawImGui() {
	if (ImGui::TreeNode("Player")) {
		const Vector3& position = GetPosition();
		ImGui::Text("position: %.2f, %.2f", position.x, position.y);
		ImGui::Text("velocity: %.3f, %.3f", velocity_.x, velocity_.y);
		ImGui::Text("grounded: %s  gliding: %s  coyote: %.2f", isGrounded_ ? "true" : "false", isGliding_ ? "true" : "false", coyoteTimer_);
		ImGui::Text("direction: %s  life: %d / %d  invincible: %.2f", (lrDirection_ == LRDirection::kRight) ? "right" : "left", life_, kMaxLife, invincibleTimer_);
		ImGui::Text("jumpBuffer: %.2f  attackJumpLock: %.2f", jumpBufferTimer_, attackJumpLockTimer_);
		ImGui::Text("catch cooldown  melee: %.2f  aim: %.2f  followPress: %.2f", meleeCooldownTimer_, aimCooldownTimer_, followPressTimer_);
		ImGui::Text("hitStop: %.2f  knockback: %.2f  controllable: %s", hitStop_.GetRemainingTime(), knockbackTimer_, isControllable_ ? "true" : "false");
		ImGui::Checkbox("invincible mode (no damage, reactions on)", &isInvincibleMode_);

		const char* attackName = "none";
		if (attackType_ == AttackType::kGround) {
			attackName = "ground";
		} else if (attackType_ == AttackType::kAir) {
			attackName = "air";
		}
		ImGui::Text("attack: %s (%.2f s, hit: %s)  aiming: %s  curve: %.2f", attackName, attackTimer_, attackHasHit_ ? "true" : "false", isAiming_ ? "true" : "false", curveOffset_);
		ImGui::Text("cursor: %.2f, %.2f", cursorWorldPosition_.x, cursorWorldPosition_.y);
		ImGui::Separator();

		boomerang_.DrawImGui();
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
