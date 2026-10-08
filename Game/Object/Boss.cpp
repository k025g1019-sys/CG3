#include "Game/Object/Boss.h"

#include "Engine/Math/Collision.h"
#include "Engine/Rendering/DebugDraw.h"
#include "Game/Object/Bit.h"
#include "Game/Object/FallingRock.h"
#include "Game/Object/Player.h"
#include "Game/Object/Shockwave.h"
#include "Game/Stage/Stage.h"

#include <cassert>
#include <cmath>
#include <numbers>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

// 体の色（状態ごと）
const Vector4 kBodyColor = { 0.45f, 0.5f, 0.6f, 1.0f };     // 通常
const Vector4 kWindupColor = { 0.9f, 0.6f, 0.3f, 1.0f };    // 予備動作（攻撃の合図）
const Vector4 kTiredColor = { 0.3f, 0.32f, 0.4f, 1.0f };    // 疲れ
const Vector4 kStaggerColor = { 1.0f, 0.9f, 0.4f, 1.0f };   // 怯み
const Vector4 kDeadColor = { 0.2f, 0.2f, 0.22f, 1.0f };     // やられ
const Vector4 kFlashColor = { 1.0f, 1.0f, 1.0f, 1.0f };     // ダメージの点滅

} // namespace

#pragma region 初期化

void Boss::Initialize(const Stage* stage, const Player* player) {
	assert(stage);
	assert(player);
	stage_ = stage;
	player_ = player;

	// 仮モデル。体は箱、盾は薄い板、コアは球
	body_.Initialize(Primitive::kCube);
	body_.SetColor(kBodyColor);

	shield_.Initialize(Primitive::kCube);
	shield_.SetColor({ 0.75f, 0.78f, 0.85f, 1.0f });

	core_.Initialize(Primitive::kSphere);
	core_.SetColor({ 1.0f, 0.5f, 0.1f, 1.0f });

	// ステージの右寄りに置き、プレイヤーのいる左を向く
	position_ = { stage_->GetRightEdge() - 8.0f, stage_->GetGroundY() + kBodyHalfHeight, 0.0f };
	facing_ = -1.0f;

	hp_ = kMaxHp;
	phase_ = 1;
	isDeathFinished_ = false;
	flashTimer_ = 0.0f;
	attackCycle_ = 0;
	isAiEnabled_ = true;
	hazards_.clear();

	std::random_device seed;
	randomEngine_.seed(seed());

	// 最初だけ長めに待つ
	behaviorRequest_.reset();
	StartBehavior(Behavior::kIdle);
	idleDuration_ = kFirstIdleDuration;

	UpdateModels();
}

#pragma endregion

#pragma region 更新

void Boss::Update() {
	// 予約された振る舞いへ切り替える
	if (behaviorRequest_) {
		StartBehavior(*behaviorRequest_);
		behaviorRequest_.reset();
	}

	switch (behavior_) {
	case Behavior::kIdle:
		UpdateIdle();
		break;
	case Behavior::kCharge:
		UpdateCharge();
		break;
	case Behavior::kTired:
		UpdateTired();
		break;
	case Behavior::kSlam:
		UpdateSlam();
		break;
	case Behavior::kRockFall:
		UpdateRockFall();
		break;
	case Behavior::kStagger:
		UpdateStagger();
		break;
	case Behavior::kDead:
		UpdateDead();
		break;
	default:
		break;
	}

	UpdatePhase();
	UpdateHazards();

	if (flashTimer_ > 0.0f) {
		flashTimer_ -= kFrameTime;
	}
	UpdateModels();

#ifndef NDEBUG
	// 開発中は判定を線で描く
	if (IsAlive()) {
		DebugDraw::DrawAABB(GetBodyBox(), { 0.4f, 0.8f, 1.0f, 1.0f });
		DebugDraw::DrawSphere(GetCoreSphere(), { 1.0f, 0.5f, 0.1f, 1.0f });
	}
#endif
}

void Boss::StartBehavior(Behavior next) {
	behavior_ = next;
	timer_ = 0.0f;

	switch (next) {
	case Behavior::kIdle:
		idleDuration_ = kIdleDuration;
		turnTimer_ = 0.0f;
		break;
	case Behavior::kCharge:
		chargeStep_ = ChargeStep::kWindup;
		chargeRemaining_ = (phase_ >= 3) ? 2 : 1;  // 第 3 段階は 2 連続
		break;
	case Behavior::kSlam:
		slamStep_ = SlamStep::kWindup;
		break;
	case Behavior::kRockFall:
		rockFallStep_ = RockFallStep::kRoar;
		break;
	case Behavior::kDead:
		KillAllHazards();
		break;
	case Behavior::kTired:
	case Behavior::kStagger:
	default:
		break;
	}
}

void Boss::UpdateIdle() {
	timer_ += kFrameTime;
	UpdateTurnToPlayer();

	if (isAiEnabled_ && timer_ >= idleDuration_) {
		ChooseNextAttack();
	}
}

void Boss::ChooseNextAttack() {
	// 突進 → 叩きつけ → 落石 の順に出す
	Behavior next = Behavior::kCharge;
	switch (attackCycle_ % kAttackPatternCount) {
	case 0:
		next = Behavior::kCharge;
		break;
	case 1:
		next = Behavior::kSlam;
		break;
	default:
		next = Behavior::kRockFall;
		break;
	}
	attackCycle_++;

	// 距離に合わないものは差し替える（遠いのに叩きつけ、近いのに突進 は不自然なため）
	const float distance = std::fabs(player_->GetPosition().x - position_.x);
	if (next == Behavior::kSlam && distance > kSlamRange) {
		next = Behavior::kCharge;
	}
	if (next == Behavior::kCharge && distance < kChargeMinDistance) {
		next = Behavior::kSlam;
	}

	// 攻撃の前にプレイヤーの方を向く
	FacePlayer();
	behaviorRequest_ = next;
}

void Boss::UpdateCharge() {
	timer_ += kFrameTime;

	switch (chargeStep_) {
	case ChargeStep::kWindup:
		if (timer_ >= kChargeWindup) {
			chargeStep_ = ChargeStep::kDash;
			timer_ = 0.0f;
			chargeStartX_ = position_.x;
		}
		break;

	case ChargeStep::kDash: {
		position_.x += facing_ * kChargeSpeed;

		// 壁に着くか、十分走ったら止まる
		bool hitWall = false;
		if (position_.x <= GetMinX()) {
			position_.x = GetMinX();
			hitWall = true;
		} else if (position_.x >= GetMaxX()) {
			position_.x = GetMaxX();
			hitWall = true;
		}
		const bool ranEnough = std::fabs(position_.x - chargeStartX_) >= kChargeDistance;
		if (hitWall || ranEnough) {
			chargeRemaining_--;
			if (chargeRemaining_ > 0) {
				// 2 連続：向き直ってもう一度
				FacePlayer();
				chargeStep_ = ChargeStep::kWindup;
				timer_ = 0.0f;
			} else {
				behaviorRequest_ = Behavior::kTired;
			}
		}
		break;
	}

	default:
		break;
	}
}

void Boss::UpdateTired() {
	timer_ += kFrameTime;
	if (timer_ >= kTiredDuration) {
		behaviorRequest_ = Behavior::kIdle;
	}
}

void Boss::UpdateSlam() {
	timer_ += kFrameTime;

	switch (slamStep_) {
	case SlamStep::kWindup:
		if (timer_ >= kSlamWindup) {
			SpawnShockwave();
			slamStep_ = SlamStep::kRecover;
			timer_ = 0.0f;
		}
		break;
	case SlamStep::kRecover:
		if (timer_ >= kSlamRecover) {
			behaviorRequest_ = Behavior::kIdle;
		}
		break;
	default:
		break;
	}
}

void Boss::UpdateRockFall() {
	timer_ += kFrameTime;

	switch (rockFallStep_) {
	case RockFallStep::kRoar:
		if (timer_ >= kRockRoar) {
			SpawnRocks();
			rockFallStep_ = RockFallStep::kRecover;
			timer_ = 0.0f;
		}
		break;
	case RockFallStep::kRecover:
		if (timer_ >= kRockRecover) {
			behaviorRequest_ = Behavior::kIdle;
		}
		break;
	default:
		break;
	}
}

void Boss::UpdateStagger() {
	timer_ += kFrameTime;
	if (timer_ >= kStaggerDuration) {
		behaviorRequest_ = Behavior::kIdle;
	}
}

void Boss::UpdateDead() {
	timer_ += kFrameTime;

	// 沈みながら倒れる
	position_.y -= kDeathSinkSpeed;
	if (timer_ >= kDeathDuration) {
		isDeathFinished_ = true;
	}
}

void Boss::FacePlayer() {
	facing_ = CalcPlayerSide();
}

void Boss::UpdateTurnToPlayer() {
	// 背後に回られたら、少し遅れて向き直る（その間は背中が狙える）
	const float side = CalcPlayerSide();
	if (side * facing_ < 0.0f) {
		turnTimer_ += kFrameTime;
		if (turnTimer_ >= kTurnDelay) {
			facing_ = side;
			turnTimer_ = 0.0f;
		}
	} else {
		turnTimer_ = 0.0f;
	}
}

void Boss::UpdatePhase() {
	if (phase_ == 1 && hp_ <= kMaxHp * 2 / 3) {
		phase_ = 2;
		SpawnBits();
	}
	if (phase_ == 2 && hp_ <= kMaxHp / 3) {
		phase_ = 3;
	}
}

void Boss::UpdateHazards() {
	// ビットが回る中心を渡してから更新する
	const Vector3 anchor = { position_.x, position_.y + kBitAnchorOffsetY, 0.0f };
	for (std::unique_ptr<Hazard>& hazard : hazards_) {
		hazard->SetAnchor(anchor);
		hazard->Update();
	}

	// 消えたものを取り除く
	for (auto it = hazards_.begin(); it != hazards_.end();) {
		if (!(*it)->IsAlive()) {
			it = hazards_.erase(it);
		} else {
			++it;
		}
	}
}

void Boss::UpdateModels() {
	// --- 体 ---
	Transform3D& body = body_.GetTransform();
	body.translate = position_;
	body.rotate = { 0.0f, 0.0f, 0.0f };
	body.scale = { kBodyHalfWidth * 2.0f, kBodyHalfHeight * 2.0f, kBodyHalfWidth * 2.0f };

	Vector4 color = kBodyColor;
	switch (behavior_) {
	case Behavior::kCharge:
		if (chargeStep_ == ChargeStep::kWindup) {
			// 後ろへ傾いて溜める
			body.rotate.z = -facing_ * 0.25f;
			color = kWindupColor;
		}
		break;
	case Behavior::kSlam:
		if (slamStep_ == SlamStep::kWindup) {
			// 伸び上がって振りかぶる
			body.scale.y *= 1.15f;
			body.translate.y += kBodyHalfHeight * 0.15f;
			color = kWindupColor;
		}
		break;
	case Behavior::kRockFall:
		if (rockFallStep_ == RockFallStep::kRoar) {
			// 吠えて震える
			body.translate.x += std::sin(timer_ * 60.0f) * 0.08f;
			color = kWindupColor;
		}
		break;
	case Behavior::kTired:
		color = kTiredColor;
		break;
	case Behavior::kStagger:
		color = kStaggerColor;
		break;
	case Behavior::kDead:
		// 倒れていく
		body.rotate.z = -facing_ * (timer_ / kDeathDuration) * 0.8f;
		color = kDeadColor;
		break;
	case Behavior::kIdle:
	default:
		break;
	}
	if (flashTimer_ > 0.0f) {
		color = kFlashColor;
	}
	body_.SetColor(color);
	body_.Update();

	// --- 盾（正面）---
	Transform3D& shield = shield_.GetTransform();
	shield.scale = { 0.3f, kBodyHalfHeight * 2.0f + 0.2f, kBodyHalfWidth * 2.0f + 0.2f };
	shield.translate = { position_.x + facing_ * kShieldOffsetX, position_.y + 0.1f, 0.0f };
	shield_.Update();

	// --- コア（背中）。少し脈打たせて目立たせる ---
	const Sphere core = GetCoreSphere();
	const float pulse = 1.0f + 0.1f * std::sin(timer_ * 6.0f);
	Transform3D& coreTransform = core_.GetTransform();
	coreTransform.translate = core.center;
	coreTransform.scale = { core.radius * 2.0f * pulse, core.radius * 2.0f * pulse, core.radius * 2.0f * pulse };
	core_.Update();
}

#pragma endregion

#pragma region 当たり判定・ダメージ

Boss::HitResult Boss::ApplyAttack(const Sphere& attackSphere, int32_t damage) {
	if (!IsAlive()) {
		return HitResult::kNone;
	}

	// 弱点のコア：ダメージ 2 倍で怯む
	if (IsCollision(attackSphere, GetCoreSphere())) {
		TakeDamage(damage * kWeakPointMultiplier);
		if (IsAlive()) {
			behaviorRequest_ = Behavior::kStagger;
		}
		return HitResult::kCoreHit;
	}

	// 体：盾が有効で正面からなら弾く
	if (IsCollision(GetBodyBox(), attackSphere)) {
		if (IsGuarding() && IsInFront(attackSphere.center)) {
			return HitResult::kBlocked;
		}
		TakeDamage(damage);
		return HitResult::kDamaged;
	}

	return HitResult::kNone;
}

void Boss::TakeDamage(int32_t damage) {
	hp_ -= damage;
	flashTimer_ = kFlashDuration;
	if (hp_ <= 0) {
		hp_ = 0;
		behaviorRequest_ = Behavior::kDead;
	}
}

AABB3D Boss::GetBodyBox() const {
	return MakeAABB(position_, { kBodyHalfWidth, kBodyHalfHeight, kBodyHalfWidth });
}

Sphere Boss::GetCoreSphere() const {
	return { { position_.x - facing_ * kCoreOffsetX, position_.y + kCoreOffsetY, 0.0f }, kCoreRadius };
}

bool Boss::IsGuarding() const {
	if (!IsAlive()) {
		return false;
	}
	return behavior_ != Behavior::kTired && behavior_ != Behavior::kStagger && behavior_ != Behavior::kDead;
}

bool Boss::IsBodyDamaging() const {
	return behavior_ == Behavior::kCharge && chargeStep_ == ChargeStep::kDash;
}

bool Boss::IsInFront(const Vector3& point) const {
	return (point.x - position_.x) * facing_ > 0.0f;
}

float Boss::CalcPlayerSide() const {
	return (player_->GetPosition().x >= position_.x) ? 1.0f : -1.0f;
}

float Boss::GetMinX() const {
	return stage_->GetLeftEdge() + kBodyHalfWidth;
}

float Boss::GetMaxX() const {
	return stage_->GetRightEdge() - kBodyHalfWidth;
}

#pragma endregion

#pragma region 攻撃物

void Boss::SpawnShockwave() {
	std::unique_ptr<Shockwave> wave = std::make_unique<Shockwave>();
	const Vector3 start = { position_.x + facing_ * (kBodyHalfWidth + 0.6f), 0.0f, 0.0f };
	wave->Initialize(start, facing_, stage_);
	hazards_.push_back(std::move(wave));
}

void Boss::SpawnRocks() {
	// プレイヤーの周りに横へ散らし、高さをずらして落ちる時間に差をつける
	std::uniform_real_distribution<float> jitter(-0.5f, 0.5f);
	const float playerX = player_->GetPosition().x;
	const float spacing = (kRockCount > 1) ? (kRockSpreadX * 2.0f / static_cast<float>(kRockCount - 1)) : 0.0f;

	for (int32_t i = 0; i < kRockCount; ++i) {
		float x = playerX - kRockSpreadX + spacing * static_cast<float>(i) + jitter(randomEngine_);
		if (x < stage_->GetLeftEdge() + 1.0f) {
			x = stage_->GetLeftEdge() + 1.0f;
		} else if (x > stage_->GetRightEdge() - 1.0f) {
			x = stage_->GetRightEdge() - 1.0f;
		}
		const float y = kRockSpawnHeight + kRockHeightStep * static_cast<float>(i);

		std::unique_ptr<FallingRock> rock = std::make_unique<FallingRock>();
		rock->Initialize({ x, y, 0.0f }, stage_);
		hazards_.push_back(std::move(rock));
	}
}

void Boss::SpawnBits() {
	const Vector3 anchor = { position_.x, position_.y + kBitAnchorOffsetY, 0.0f };
	for (int32_t i = 0; i < kBitCount; ++i) {
		const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(kBitCount);
		std::unique_ptr<Bit> bit = std::make_unique<Bit>();
		bit->Initialize(anchor, angle);
		hazards_.push_back(std::move(bit));
	}
}

void Boss::KillAllHazards() {
	for (std::unique_ptr<Hazard>& hazard : hazards_) {
		hazard->Kill();
	}
}

#pragma endregion

#pragma region 描画

void Boss::Draw() const {
	body_.Draw();
	core_.Draw();
	if (IsGuarding()) {
		shield_.Draw();
	}
	for (const std::unique_ptr<Hazard>& hazard : hazards_) {
		hazard->Draw();
	}
}

const char* Boss::GetBehaviorName() const {
	switch (behavior_) {
	case Behavior::kIdle:
		return "idle";
	case Behavior::kCharge:
		return (chargeStep_ == ChargeStep::kWindup) ? "charge(windup)" : "charge(dash)";
	case Behavior::kTired:
		return "tired";
	case Behavior::kSlam:
		return "slam";
	case Behavior::kRockFall:
		return "rockfall";
	case Behavior::kStagger:
		return "stagger";
	case Behavior::kDead:
		return "dead";
	default:
		return "?";
	}
}

#ifdef USE_IMGUI
void Boss::DrawImGui() {
	if (ImGui::TreeNode("Boss")) {
		ImGui::Text("hp: %d / %d  phase: %d  behavior: %s", hp_, kMaxHp, phase_, GetBehaviorName());
		ImGui::Text("position: %.2f, %.2f  facing: %s  guarding: %s  hazards: %d",
			position_.x, position_.y, (facing_ > 0.0f) ? "right" : "left", IsGuarding() ? "true" : "false", static_cast<int>(hazards_.size()));
		ImGui::Checkbox("AI enabled", &isAiEnabled_);
		ImGui::Separator();
		ImGui::DragFloat("idleDuration", &kIdleDuration, 0.05f, 0.0f, 10.0f);
		ImGui::DragFloat("turnDelay", &kTurnDelay, 0.05f, 0.0f, 5.0f);
		ImGui::DragFloat("chargeWindup", &kChargeWindup, 0.05f, 0.0f, 5.0f);
		ImGui::DragFloat("chargeSpeed", &kChargeSpeed, 0.01f, 0.0f, 2.0f);
		ImGui::DragFloat("chargeDistance", &kChargeDistance, 0.5f, 1.0f, 60.0f);
		ImGui::DragFloat("tiredDuration", &kTiredDuration, 0.05f, 0.0f, 10.0f);
		ImGui::DragFloat("slamWindup", &kSlamWindup, 0.05f, 0.0f, 5.0f);
		ImGui::DragFloat("slamRecover", &kSlamRecover, 0.05f, 0.0f, 5.0f);
		ImGui::DragFloat("rockRoar", &kRockRoar, 0.05f, 0.0f, 5.0f);
		ImGui::DragFloat("rockRecover", &kRockRecover, 0.05f, 0.0f, 5.0f);
		ImGui::DragFloat("staggerDuration", &kStaggerDuration, 0.05f, 0.0f, 5.0f);
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
