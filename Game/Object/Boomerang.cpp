#include "Game/Object/Boomerang.h"

#include "Engine/Math/Curve.h"
#include "Engine/Math/Matrix4x4.h"
#include "Game/Util/Easing.h"
#include "Game/Util/GlobalVariables.h"

#include <algorithm>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

const Vector3 kModelScale = { 1.0f, 0.2f, 0.2f };  // 普通の大きさのときのモデルの拡大率（細長い立方体）

} // namespace

#pragma region 調整値

void Boomerang::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Boomerang";
	globalVariables->CreateGroup(groupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(groupName, "Damage", kDamage);
	globalVariables->AddItem(groupName, "OutwardDuration", kOutwardDuration);
	globalVariables->AddItem(groupName, "ReturnStartSpeed", kReturnStartSpeed);
	globalVariables->AddItem(groupName, "ReturnAcceleration", kReturnAcceleration);
	globalVariables->AddItem(groupName, "ReturnMaxSpeed", kReturnMaxSpeed);
	globalVariables->AddItem(groupName, "CatchDistance", kCatchDistance);
	globalVariables->AddItem(groupName, "Radius", kRadius);
	globalVariables->AddItem(groupName, "SpinSpeed", kSpinSpeed);
	globalVariables->AddItem(groupName, "CurvePeakRate", kCurvePeakRate);
	globalVariables->AddItem(groupName, "DownCurveVerticalRate", kDownCurveVerticalRate);
	globalVariables->AddItem(groupName, "BlockEffectCooldown", kBlockEffectCooldown);
	globalVariables->AddItem(groupName, "FollowMaxSpeed", kFollowMaxSpeed);
	globalVariables->AddItem(groupName, "FollowAccelTime", kFollowAccelTime);
	globalVariables->AddItem(groupName, "FollowSlowDistance", kFollowSlowDistance);
	globalVariables->AddItem(groupName, "FollowBounceSpeed", kFollowBounceSpeed);
	globalVariables->AddItem(groupName, "FollowBounceDuration", kFollowBounceDuration);
	globalVariables->AddItem(groupName, "FollowHalfHitCount", kFollowHalfHitCount);
	globalVariables->AddItem(groupName, "BurstDuration", kBurstDuration);
	globalVariables->AddItem(groupName, "BurstScale", kBurstScale);
	globalVariables->AddItem(groupName, "BurstDamageMultiplier", kBurstDamageMultiplier);
	globalVariables->AddItem(groupName, "BurstRecoverDuration", kBurstRecoverDuration);
	globalVariables->AddItem(groupName, "BurstReturnScale", kBurstReturnScale);
	globalVariables->AddItem(groupName, "BurstReturnSpeedRate", kBurstReturnSpeedRate);
}

void Boomerang::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Boomerang";
	kDamage = globalVariables->GetIntValue(groupName, "Damage");
	kOutwardDuration = globalVariables->GetFloatValue(groupName, "OutwardDuration");
	kReturnStartSpeed = globalVariables->GetFloatValue(groupName, "ReturnStartSpeed");
	kReturnAcceleration = globalVariables->GetFloatValue(groupName, "ReturnAcceleration");
	kReturnMaxSpeed = globalVariables->GetFloatValue(groupName, "ReturnMaxSpeed");
	kCatchDistance = globalVariables->GetFloatValue(groupName, "CatchDistance");
	kRadius = globalVariables->GetFloatValue(groupName, "Radius");
	kSpinSpeed = globalVariables->GetFloatValue(groupName, "SpinSpeed");
	kCurvePeakRate = globalVariables->GetFloatValue(groupName, "CurvePeakRate");
	kDownCurveVerticalRate = globalVariables->GetFloatValue(groupName, "DownCurveVerticalRate");
	kBlockEffectCooldown = globalVariables->GetFloatValue(groupName, "BlockEffectCooldown");
	kFollowMaxSpeed = globalVariables->GetFloatValue(groupName, "FollowMaxSpeed");
	kFollowAccelTime = globalVariables->GetFloatValue(groupName, "FollowAccelTime");
	kFollowSlowDistance = globalVariables->GetFloatValue(groupName, "FollowSlowDistance");
	kFollowBounceSpeed = globalVariables->GetFloatValue(groupName, "FollowBounceSpeed");
	kFollowBounceDuration = globalVariables->GetFloatValue(groupName, "FollowBounceDuration");
	kFollowHalfHitCount = globalVariables->GetIntValue(groupName, "FollowHalfHitCount");
	kBurstDuration = globalVariables->GetFloatValue(groupName, "BurstDuration");
	kBurstScale = globalVariables->GetFloatValue(groupName, "BurstScale");
	kBurstDamageMultiplier = globalVariables->GetIntValue(groupName, "BurstDamageMultiplier");
	kBurstRecoverDuration = globalVariables->GetFloatValue(groupName, "BurstRecoverDuration");
	kBurstReturnScale = globalVariables->GetFloatValue(groupName, "BurstReturnScale");
	kBurstReturnSpeedRate = globalVariables->GetFloatValue(groupName, "BurstReturnSpeedRate");
}

#pragma endregion

#pragma region 初期化

void Boomerang::Initialize(float groundY) {
	// 仮モデル（細長い立方体を回して見せる）。モデルができたら差し替える
	model_.Initialize(Primitive::kCube);
	model_.SetColor({ 1.0f, 0.85f, 0.3f, 1.0f });
	model_.GetTransform().scale = kModelScale;

	groundY_ = groundY;
	Reset();
}

void Boomerang::Reset() {
	phase_ = Phase::kIdle;
	position_ = { 0.0f, 0.0f, 0.0f };
	timer_ = 0.0f;
	speed_ = 0.0f;
	size_ = 1.0f;
	blockEffectTimer_ = 0.0f;
	bounceTimer_ = 0.0f;
	isFollowUsed_ = false;
	followHitCount_ = 0;
	isBurstUsed_ = false;
	hasBurstHit_ = false;
}

#pragma endregion

#pragma region 更新

void Boomerang::Update(const Vector3& ownerPosition) {
	// 盾に弾かれた演出の間隔
	if (blockEffectTimer_ > 0.0f) {
		blockEffectTimer_ -= kFrameTime;
	}

	switch (phase_) {
	case Phase::kOutward:
		UpdateOutward();
		break;
	case Phase::kReturn:
		UpdateReturn(ownerPosition);
		break;
	case Phase::kFollow:
		UpdateFollow();
		break;
	case Phase::kBurst:
		UpdateBurst();
		break;
	case Phase::kBurstRecover:
		UpdateBurstRecover();
		break;
	case Phase::kIdle:
	default:
		break;
	}

	// 見た目：位置と大きさを合わせ、飛行中は回し続ける
	Transform3D& transform = model_.GetTransform();
	transform.translate = position_;
	transform.scale = kModelScale * size_;
	if (IsFlying()) {
		transform.rotate.z += kSpinSpeed;
	}
	model_.Update();
}

void Boomerang::UpdateModel() {
	model_.Update();
}

void Boomerang::Throw(const Vector3& start, const Vector3& target, float curveOffset) {
	if (IsFlying()) {
		return;
	}

	// 前の投げの状態（大きさ・追従・大技など）を消してから投げる
	Reset();
	controlPoints_ = MakeControlPoints(start, target, curveOffset);
	position_ = start;
	phase_ = Phase::kOutward;
}

void Boomerang::Repel(const Vector3& pushDirection) {
	// 往路なら、そこで折り返して戻る
	if (phase_ == Phase::kOutward) {
		StartReturn();
		return;
	}

	// 追従中なら、押し返されながら追従を続ける（滑り終えたら、また動き出しから追いかける）
	if (phase_ == Phase::kFollow) {
		bounceDirection_ = pushDirection;
		bounceTimer_ = kFollowBounceDuration;
		followTimer_ = 0.0f;
	}
}

void Boomerang::StartFollow(const Vector3& target) {
	if (!CanStartFollow()) {
		return;
	}
	phase_ = Phase::kFollow;
	followTarget_ = target;
	followTimer_ = 0.0f;
	bounceTimer_ = 0.0f;

	// 追従を使った投げは、戻ってくるまでダメージが減る
	isFollowUsed_ = true;
}

void Boomerang::EndFollow() {
	if (phase_ == Phase::kFollow) {
		StartReturn();
	}
}

void Boomerang::StartBurst() {
	if (phase_ != Phase::kFollow) {
		return;
	}
	phase_ = Phase::kBurst;
	timer_ = 0.0f;
	isBurstUsed_ = true;
	hasBurstHit_ = false;
}

std::array<Vector3, 3> Boomerang::MakeControlPoints(const Vector3& start, const Vector3& target, float curveOffset) {
	// 始点と目標点を結ぶ線に垂直な単位ベクトル（線を 90 度回したもの）。上側（Y が正）を正の向きにそろえる
	const Vector3 chord = target - start;
	Vector3 normal = { -chord.y, chord.x, 0.0f };
	if (Length(normal) < 1e-6f) {
		// 始点と目標点が同じ場所なら真上を使う
		normal = { 0.0f, 1.0f, 0.0f };
	} else {
		normal = Normalize(normal);
		if (normal.y < 0.0f || (normal.y == 0.0f && normal.x < 0.0f)) {
			normal = -normal;
		}
	}

	// 2 次ベジェ曲線は制御点の方へ半分だけ寄るので、制御点は 2 倍ずらして曲線の膨らみを curveOffset にそろえる。
	// ずらす元の点を線の真ん中から目標点寄りにすると、曲線の山も目標点寄りになる（最初はまっすぐ気味、最後に回り込む）
	Vector3 middle = start + chord * kCurvePeakRate + normal * (curveOffset * 2.0f);

	// 下向きに曲げたときは、制御点を目標点の真下へ寄せる。2 次ベジェ曲線の終わりの向きは「制御点 → 目標点」なので、
	// 真下に置くと最後は真上へ上がって目標点に入る（線に垂直に膨らませるだけだと、終わりが横へそれて入る）
	if (curveOffset < 0.0f) {
		const Vector3 below = { target.x, target.y + curveOffset * 2.0f, 0.0f };
		middle = Lerp(middle, below, kDownCurveVerticalRate);
	}
	return std::array<Vector3, 3>{ start, middle, target };
}

void Boomerang::UpdateOutward() {
	timer_ += kFrameTime;
	const float t = std::clamp(timer_ / kOutwardDuration, 0.0f, 1.0f);

	// 終わりにかけて減速しながら曲線をなぞる
	position_ = Bezier(controlPoints_[0], controlPoints_[1], controlPoints_[2], EaseOutQuad(t));

	// 地面に触れたら、そこで折り返す（盾に弾かれたときと同じ）
	if (ResolveGroundCollision()) {
		StartReturn();
		return;
	}

	// 目標点に着いたら復路へ
	if (t >= 1.0f) {
		StartReturn();
	}
}

bool Boomerang::ResolveGroundCollision() {
	// 判定の球の下端が地面に届いたら、地面の上へ押し戻す（今の大きさの判定で見る）
	const float minY = groundY_ + kRadius * size_;
	if (position_.y <= minY) {
		position_.y = minY;
		return true;
	}
	return false;
}

void Boomerang::StartReturn() {
	phase_ = Phase::kReturn;
	speed_ = kReturnStartSpeed * CalcReturnSpeedRate();
}

float Boomerang::CalcReturnSpeedRate() const {
	return isBurstUsed_ ? kBurstReturnSpeedRate : 1.0f;
}

void Boomerang::UpdateReturn(const Vector3& ownerPosition) {
	const Vector3 toOwner = ownerPosition - position_;
	const float distance = Length(toOwner);

	// 手元まで戻ったら回収
	if (distance <= kCatchDistance) {
		phase_ = Phase::kIdle;
		position_ = ownerPosition;
		return;
	}

	// 加速しながら持ち主へ向かう（行き過ぎないよう、1 フレームの移動量は残りの距離で頭打ち）。大技の後は速く戻る
	const float speedRate = CalcReturnSpeedRate();
	speed_ = std::clamp(speed_ + kReturnAcceleration * speedRate, 0.0f, kReturnMaxSpeed * speedRate);
	const float step = (speed_ < distance) ? speed_ : distance;
	position_ += Normalize(toOwner) * step;

	// 復路でも地面にはもぐらない（高さだけ止めて、そのまま持ち主へ向かう）
	ResolveGroundCollision();
}

void Boomerang::UpdateFollow() {
	// 盾に押し返されている間は、押し返される向きへ滑る（終わりにかけて遅くなる）
	if (bounceTimer_ > 0.0f) {
		const float rate = (kFollowBounceDuration > 0.0f) ? (bounceTimer_ / kFollowBounceDuration) : 0.0f;
		position_ += bounceDirection_ * (kFollowBounceSpeed * rate);
		bounceTimer_ -= kFrameTime;
		ResolveGroundCollision();
		return;
	}

	// カーソルへ向かう（ease-in-out の動き）。動き出しは FollowAccelTime 秒かけて速さを 0 から上限まで上げ、
	// 止まり際はカーソルまでの距離が FollowSlowDistance より近いと、距離に比例して遅くする
	followTimer_ += kFrameTime;
	float accelRate = 1.0f;
	if (kFollowAccelTime > 0.0f) {
		accelRate = EaseInOut(std::clamp(followTimer_ / kFollowAccelTime, 0.0f, 1.0f));
	}
	const Vector3 toTarget = followTarget_ - position_;
	const float distance = Length(toTarget);
	float slowRate = 1.0f;
	if (kFollowSlowDistance > 0.0f) {
		slowRate = std::clamp(distance / kFollowSlowDistance, 0.0f, 1.0f);
	}

	// 行き過ぎないよう、1 フレームの移動量は残りの距離で頭打ち
	const float speed = kFollowMaxSpeed * accelRate * slowRate;
	const float step = (speed < distance) ? speed : distance;
	if (distance > 0.0f) {
		position_ += toTarget * (step / distance);
	}

	// カーソルが地面より下にあっても、地面にはもぐらない
	ResolveGroundCollision();
}

void Boomerang::UpdateBurst() {
	// その場で止まったまま大きくなる（最初が速く、終わりにかけてゆっくり）
	timer_ += kFrameTime;
	const float t = (kBurstDuration > 0.0f) ? std::clamp(timer_ / kBurstDuration, 0.0f, 1.0f) : 1.0f;
	size_ = Lerp(1.0f, kBurstScale, EaseOutQuad(t));
	if (t >= 1.0f) {
		phase_ = Phase::kBurstRecover;
		timer_ = 0.0f;
	}
}

void Boomerang::UpdateBurstRecover() {
	// 止まったまま、戻るときの小ささまで縮む（この間は当たらない）。縮み終えたら速く戻り始める
	timer_ += kFrameTime;
	const float t = (kBurstRecoverDuration > 0.0f) ? std::clamp(timer_ / kBurstRecoverDuration, 0.0f, 1.0f) : 1.0f;
	size_ = Lerp(kBurstScale, kBurstReturnScale, EaseOutQuad(t));
	if (t >= 1.0f) {
		StartReturn();
	}
}

bool Boomerang::CanStartFollow() const {
	// 往路か、追従を使っていない復路のときだけ（追従をやめた後の戻りと、大技の後は始めない）
	const bool isOutwardOrReturn = (phase_ == Phase::kOutward) || (phase_ == Phase::kReturn);
	return isOutwardOrReturn && !isFollowUsed_ && !isBurstUsed_;
}

bool Boomerang::CanHit() const {
	switch (phase_) {
	case Phase::kOutward:
	case Phase::kFollow:
		return true;
	case Phase::kReturn:
		// 大技の後の戻りは当たらない
		return !isBurstUsed_;
	case Phase::kBurst:
		// 大技の一撃は 1 回だけ
		return !hasBurstHit_;
	case Phase::kBurstRecover:
	case Phase::kIdle:
	default:
		return false;
	}
}

int32_t Boomerang::CalcDamage() const {
	// 大技の一撃は、普通の何倍か
	if (phase_ == Phase::kBurst) {
		return kDamage * kBurstDamageMultiplier;
	}

	// 追従を使った投げは、戻ってくるまで半分。決めた回数だけ当てた後は、さらに半分（整数なので切り捨て）
	if (isFollowUsed_) {
		if (followHitCount_ < kFollowHalfHitCount) {
			return kDamage / 2;
		}
		return kDamage / 4;
	}
	return kDamage;
}

void Boomerang::OnHit() {
	// 大技の一撃を当てたら、もう当たらない
	if (phase_ == Phase::kBurst) {
		hasBurstHit_ = true;
		return;
	}

	// 追従を使った投げは、当たった回数を数える（ダメージをさらに半分にするかの判断に使う）
	if (isFollowUsed_) {
		followHitCount_++;
	}
}

#pragma endregion

#pragma region 描画

void Boomerang::Draw() const {
	// 手元にあるときは描かない（プレイヤーが持っている想定）
	if (!IsFlying()) {
		return;
	}
	model_.Draw();
}

const char* Boomerang::GetPhaseName() const {
	switch (phase_) {
	case Phase::kOutward:
		return "outward";
	case Phase::kReturn:
		return isBurstUsed_ ? "return (after burst)" : "return";
	case Phase::kFollow:
		return "follow";
	case Phase::kBurst:
		return "burst";
	case Phase::kBurstRecover:
		return "burst recover";
	case Phase::kIdle:
	default:
		return "idle";
	}
}

#ifdef USE_IMGUI
void Boomerang::DrawImGui() {
	if (ImGui::TreeNode("Boomerang")) {
		ImGui::Text("phase: %s  position: %.2f, %.2f  speed: %.3f  size: %.2f",
			GetPhaseName(), position_.x, position_.y, speed_, size_);
		ImGui::Text("followUsed: %s  followHits: %d  burstUsed: %s  burstHit: %s  damage: %d",
			isFollowUsed_ ? "true" : "false", followHitCount_, isBurstUsed_ ? "true" : "false",
			hasBurstHit_ ? "true" : "false", CalcDamage());
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
