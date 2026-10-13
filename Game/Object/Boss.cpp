#include "Game/Object/Boss.h"

#include "Engine/Math/Collision.h"
#include "Engine/Rendering/DebugDraw.h"
#include "Game/Object/Bit.h"
#include "Game/Object/FallingRock.h"
#include "Game/Object/Player.h"
#include "Game/Object/Shockwave.h"
#include "Game/Stage/Stage.h"
#include "Game/Util/GlobalVariables.h"

#include <algorithm>
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
const Vector4 kShieldColor = { 0.75f, 0.78f, 0.85f, 1.0f }; // 盾

// 予備動作の印（半透明）
const Vector4 kChargeMarkColor = { 1.0f, 0.3f, 0.2f, 0.4f };  // 突進の進路
const Vector4 kSlamMarkColor = { 1.0f, 0.6f, 0.2f, 0.4f };    // 叩きつけの範囲
const Vector4 kRockMarkColor = { 1.0f, 0.7f, 0.3f, 0.4f };    // 落石の落下位置
const float kMarkHeight = 0.06f;                              // 印を床から浮かせる高さ（床と重なってちらつかないように）
const float kRockMarkSize = 1.4f;                             // 落石の印の直径

} // namespace

#pragma region 調整値

void Boss::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Boss";
	globalVariables->CreateGroup(groupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(groupName, "MaxHp", kMaxHp);
	globalVariables->AddItem(groupName, "WeakPointMultiplier", kWeakPointMultiplier);
	globalVariables->AddItem(groupName, "RockCount", kRockCount);
	globalVariables->AddItem(groupName, "BitCount", kBitCount);
	globalVariables->AddItem(groupName, "BodyHalfWidth", kBodyHalfWidth);
	globalVariables->AddItem(groupName, "BodyHalfHeight", kBodyHalfHeight);
	globalVariables->AddItem(groupName, "ShieldOffsetX", kShieldOffsetX);
	globalVariables->AddItem(groupName, "CoreOffsetX", kCoreOffsetX);
	globalVariables->AddItem(groupName, "CoreOffsetY", kCoreOffsetY);
	globalVariables->AddItem(groupName, "CoreRadius", kCoreRadius);
	globalVariables->AddItem(groupName, "FirstIdleDuration", kFirstIdleDuration);
	globalVariables->AddItem(groupName, "IdleDuration", kIdleDuration);
	globalVariables->AddItem(groupName, "TurnDelay", kTurnDelay);
	globalVariables->AddItem(groupName, "ChargeWindup", kChargeWindup);
	globalVariables->AddItem(groupName, "ChargeSpeed", kChargeSpeed);
	globalVariables->AddItem(groupName, "ChargeDistance", kChargeDistance);
	globalVariables->AddItem(groupName, "ChargeMinDistance", kChargeMinDistance);
	globalVariables->AddItem(groupName, "TiredDuration", kTiredDuration);
	globalVariables->AddItem(groupName, "SlamWindup", kSlamWindup);
	globalVariables->AddItem(groupName, "SlamRecover", kSlamRecover);
	globalVariables->AddItem(groupName, "SlamRange", kSlamRange);
	globalVariables->AddItem(groupName, "RockRoar", kRockRoar);
	globalVariables->AddItem(groupName, "RockRecover", kRockRecover);
	globalVariables->AddItem(groupName, "RockSpawnHeight", kRockSpawnHeight);
	globalVariables->AddItem(groupName, "RockHeightStep", kRockHeightStep);
	globalVariables->AddItem(groupName, "RockSpreadX", kRockSpreadX);
	globalVariables->AddItem(groupName, "StaggerDuration", kStaggerDuration);
	globalVariables->AddItem(groupName, "StaggerCooldown", kStaggerCooldown);
	globalVariables->AddItem(groupName, "DeathDuration", kDeathDuration);
	globalVariables->AddItem(groupName, "DeathSinkSpeed", kDeathSinkSpeed);
	globalVariables->AddItem(groupName, "FlashDuration", kFlashDuration);
	globalVariables->AddItem(groupName, "ShieldFlashDuration", kShieldFlashDuration);
	globalVariables->AddItem(groupName, "BitAnchorOffsetY", kBitAnchorOffsetY);
}

void Boss::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Boss";
	kMaxHp = globalVariables->GetIntValue(groupName, "MaxHp");
	kWeakPointMultiplier = globalVariables->GetIntValue(groupName, "WeakPointMultiplier");
	kRockCount = globalVariables->GetIntValue(groupName, "RockCount");
	kBitCount = globalVariables->GetIntValue(groupName, "BitCount");
	kBodyHalfWidth = globalVariables->GetFloatValue(groupName, "BodyHalfWidth");
	kBodyHalfHeight = globalVariables->GetFloatValue(groupName, "BodyHalfHeight");
	kShieldOffsetX = globalVariables->GetFloatValue(groupName, "ShieldOffsetX");
	kCoreOffsetX = globalVariables->GetFloatValue(groupName, "CoreOffsetX");
	kCoreOffsetY = globalVariables->GetFloatValue(groupName, "CoreOffsetY");
	kCoreRadius = globalVariables->GetFloatValue(groupName, "CoreRadius");
	kFirstIdleDuration = globalVariables->GetFloatValue(groupName, "FirstIdleDuration");
	kIdleDuration = globalVariables->GetFloatValue(groupName, "IdleDuration");
	kTurnDelay = globalVariables->GetFloatValue(groupName, "TurnDelay");
	kChargeWindup = globalVariables->GetFloatValue(groupName, "ChargeWindup");
	kChargeSpeed = globalVariables->GetFloatValue(groupName, "ChargeSpeed");
	kChargeDistance = globalVariables->GetFloatValue(groupName, "ChargeDistance");
	kChargeMinDistance = globalVariables->GetFloatValue(groupName, "ChargeMinDistance");
	kTiredDuration = globalVariables->GetFloatValue(groupName, "TiredDuration");
	kSlamWindup = globalVariables->GetFloatValue(groupName, "SlamWindup");
	kSlamRecover = globalVariables->GetFloatValue(groupName, "SlamRecover");
	kSlamRange = globalVariables->GetFloatValue(groupName, "SlamRange");
	kRockRoar = globalVariables->GetFloatValue(groupName, "RockRoar");
	kRockRecover = globalVariables->GetFloatValue(groupName, "RockRecover");
	kRockSpawnHeight = globalVariables->GetFloatValue(groupName, "RockSpawnHeight");
	kRockHeightStep = globalVariables->GetFloatValue(groupName, "RockHeightStep");
	kRockSpreadX = globalVariables->GetFloatValue(groupName, "RockSpreadX");
	kStaggerDuration = globalVariables->GetFloatValue(groupName, "StaggerDuration");
	kStaggerCooldown = globalVariables->GetFloatValue(groupName, "StaggerCooldown");
	kDeathDuration = globalVariables->GetFloatValue(groupName, "DeathDuration");
	kDeathSinkSpeed = globalVariables->GetFloatValue(groupName, "DeathSinkSpeed");
	kFlashDuration = globalVariables->GetFloatValue(groupName, "FlashDuration");
	kShieldFlashDuration = globalVariables->GetFloatValue(groupName, "ShieldFlashDuration");
	kBitAnchorOffsetY = globalVariables->GetFloatValue(groupName, "BitAnchorOffsetY");
}

#pragma endregion

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
	shield_.SetColor(kShieldColor);

	core_.Initialize(Primitive::kSphere);
	core_.SetColor({ 1.0f, 0.5f, 0.1f, 1.0f });

	// 予備動作の印（半透明。ライティングなしで色をそのまま出す）
	attackMark_.Initialize(Primitive::kCube, LightingMode::kNone);
	for (Object3D& mark : rockMarks_) {
		mark.Initialize(Primitive::kSphere, LightingMode::kNone);
	}
	isAttackMarkVisible_ = false;
	visibleRockMarkCount_ = 0;
	rockTargetsX_.clear();

	// ステージの右寄りに置き、プレイヤーのいる左を向く
	position_ = { stage_->GetRightEdge() - 8.0f, stage_->GetGroundY() + kBodyHalfHeight, 0.0f };
	facing_ = -1.0f;

	hp_ = kMaxHp;
	phase_ = 1;
	isDeathFinished_ = false;
	flashTimer_ = 0.0f;
	shieldFlashTimer_ = 0.0f;
	staggerCooldownTimer_ = 0.0f;
	facePlayerRequest_ = false;
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
	UpdateMarks();
}

#pragma endregion

#pragma region 更新

void Boss::Update() {
	// 殴られたときの向き直り（当たり判定の中で向きを変えると、同じフレームのコアの位置がずれるので次の更新で行う）
	if (facePlayerRequest_) {
		FacePlayer();
		facePlayerRequest_ = false;
	}

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
	if (shieldFlashTimer_ > 0.0f) {
		shieldFlashTimer_ -= kFrameTime;
	}
	if (staggerCooldownTimer_ > 0.0f) {
		staggerCooldownTimer_ -= kFrameTime;
	}
	UpdateModels();
	UpdateMarks();

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
		PlanRocks();  // 落下位置を先に決め、吠えている間は印を出す
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
		// 立ち直ったらすぐプレイヤーの方を向く（背中を殴られ続けないように）
		FacePlayer();
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
	// 前のフレームに消えたものを先に取り除く（消えたフレームは描かないので、最後に描いたフレームの
	// 描画が GPU で終わってから壊すことになる。描画中のものの定数バッファを壊さないため）
	for (auto it = hazards_.begin(); it != hazards_.end();) {
		if (!(*it)->IsAlive()) {
			it = hazards_.erase(it);
		} else {
			++it;
		}
	}

	// ビットが回る中心を渡してから更新する
	const Vector3 anchor = { position_.x, position_.y + kBitAnchorOffsetY, 0.0f };
	for (std::unique_ptr<Hazard>& hazard : hazards_) {
		hazard->SetAnchor(anchor);
		hazard->Update();
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

	// --- 盾（正面）。弾いた直後は白く光る ---
	Transform3D& shield = shield_.GetTransform();
	shield.scale = { 0.3f, kBodyHalfHeight * 2.0f + 0.2f, kBodyHalfWidth * 2.0f + 0.2f };
	shield.translate = GetShieldPosition();
	shield_.SetColor((shieldFlashTimer_ > 0.0f) ? kFlashColor : kShieldColor);
	shield_.Update();

	// --- コア（背中）。少し脈打たせて目立たせる ---
	const Sphere core = GetCoreSphere();
	const float pulse = 1.0f + 0.1f * std::sin(timer_ * 6.0f);
	Transform3D& coreTransform = core_.GetTransform();
	coreTransform.translate = core.center;
	coreTransform.scale = { core.radius * 2.0f * pulse, core.radius * 2.0f * pulse, core.radius * 2.0f * pulse };
	core_.Update();
}

void Boss::UpdateMarks() {
	const float groundY = stage_->GetGroundY();

	// --- 突進・叩きつけの予備動作：これから進む範囲を床の上の半透明の帯で示す ---
	isAttackMarkVisible_ = false;
	float length = 0.0f;
	float progress = 0.0f;
	Vector4 color = kChargeMarkColor;
	if (behavior_ == Behavior::kCharge && chargeStep_ == ChargeStep::kWindup) {
		// 走る距離（壁までの方が短ければそちら）
		const float toWall = (facing_ > 0.0f) ? (GetMaxX() - position_.x) : (position_.x - GetMinX());
		length = (toWall < kChargeDistance) ? toWall : kChargeDistance;
		progress = timer_ / kChargeWindup;
		isAttackMarkVisible_ = true;
	} else if (behavior_ == Behavior::kSlam && slamStep_ == SlamStep::kWindup) {
		length = kSlamRange;
		progress = timer_ / kSlamWindup;
		color = kSlamMarkColor;
		isAttackMarkVisible_ = true;
	}
	if (isAttackMarkVisible_ && length > 0.0f) {
		// 予備動作が進むほど濃くする
		color.w = 0.15f + 0.35f * std::clamp(progress, 0.0f, 1.0f);
		Transform3D& mark = attackMark_.GetTransform();
		mark.translate = { position_.x + facing_ * (kBodyHalfWidth + length * 0.5f), groundY + kMarkHeight, 0.0f };
		mark.scale = { length, kMarkHeight * 2.0f, kBodyHalfWidth * 2.0f };
		attackMark_.SetColor(color);
		attackMark_.Update();
	} else {
		isAttackMarkVisible_ = false;
	}

	// --- 落石の予備動作：落ちる位置に半透明の円盤を置き、点滅させる ---
	visibleRockMarkCount_ = 0;
	if (behavior_ == Behavior::kRockFall && rockFallStep_ == RockFallStep::kRoar) {
		const float blink = 0.5f + 0.5f * std::sin(timer_ * 25.0f);
		Vector4 rockColor = kRockMarkColor;
		rockColor.w = 0.2f + 0.3f * blink;
		const int32_t count = static_cast<int32_t>(rockTargetsX_.size());
		for (int32_t i = 0; i < count && i < kMaxRockMarkers; ++i) {
			Transform3D& mark = rockMarks_[i].GetTransform();
			mark.translate = { rockTargetsX_[i], groundY + kMarkHeight, 0.0f };
			mark.scale = { kRockMarkSize, kMarkHeight * 2.0f, kRockMarkSize };
			rockMarks_[i].SetColor(rockColor);
			rockMarks_[i].Update();
			visibleRockMarkCount_++;
		}
	}
}

void Boss::UpdateModel() {
	UpdateModels();
	UpdateMarks();
	for (std::unique_ptr<Hazard>& hazard : hazards_) {
		if (hazard->IsAlive()) {
			hazard->UpdateModel();
		}
	}
}

#pragma endregion

#pragma region 当たり判定・ダメージ

Boss::HitResult Boss::ApplyAttack(const Sphere& attackSphere, int32_t damage, const Vector3& attackerPosition) {
	if (!IsAlive()) {
		return HitResult::kNone;
	}
	const bool touchesCore = IsCollision(attackSphere, GetCoreSphere());
	const bool touchesBody = IsCollision(GetBodyBox(), attackSphere);
	if (!touchesCore && !touchesBody) {
		return HitResult::kNone;
	}

	// 盾が有効で、攻撃してきた側が正面にいるなら、判定がコアに届いていても弾く（正面から体越しにコアを殴れないようにする）
	if (IsGuarding() && IsInFront(attackerPosition)) {
		shieldFlashTimer_ = kShieldFlashDuration;
		return HitResult::kBlocked;
	}

	// 弱点のコア：ダメージ 2 倍。怯みは間隔を空ける（連続で怯ませて封じられないように）
	if (touchesCore) {
		TakeDamage(damage * kWeakPointMultiplier);
		if (IsAlive()) {
			if (staggerCooldownTimer_ <= 0.0f) {
				behaviorRequest_ = Behavior::kStagger;
				staggerCooldownTimer_ = kStaggerCooldown;
			} else if (behavior_ == Behavior::kIdle) {
				facePlayerRequest_ = true;  // 怯まないときは殴られた方へ向き直る（次の更新で）
			}
		}
		return HitResult::kCoreHit;
	}

	// 体（背後からか、盾が下がっている間）。待機中に殴られたら、その方へ向き直る（次の更新で）
	TakeDamage(damage);
	if (IsAlive() && behavior_ == Behavior::kIdle) {
		facePlayerRequest_ = true;
	}
	return HitResult::kDamaged;
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

Vector3 Boss::GetShieldPosition() const {
	return { position_.x + facing_ * kShieldOffsetX, position_.y + 0.1f, 0.0f };
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

void Boss::PlanRocks() {
	// プレイヤーの周りに横へ散らして落下位置を決める（個数は印の数の上限まで）
	rockTargetsX_.clear();
	std::uniform_real_distribution<float> jitter(-0.5f, 0.5f);
	const float playerX = player_->GetPosition().x;
	const int32_t count = (kRockCount < kMaxRockMarkers) ? kRockCount : kMaxRockMarkers;
	const float spacing = (count > 1) ? (kRockSpreadX * 2.0f / static_cast<float>(count - 1)) : 0.0f;

	for (int32_t i = 0; i < count; ++i) {
		float x = playerX - kRockSpreadX + spacing * static_cast<float>(i) + jitter(randomEngine_);
		if (x < stage_->GetLeftEdge() + 1.0f) {
			x = stage_->GetLeftEdge() + 1.0f;
		} else if (x > stage_->GetRightEdge() - 1.0f) {
			x = stage_->GetRightEdge() - 1.0f;
		}
		rockTargetsX_.push_back(x);
	}
}

void Boss::SpawnRocks() {
	// 決めておいた位置に、高さをずらして落ちる時間に差をつけて出す
	for (size_t i = 0; i < rockTargetsX_.size(); ++i) {
		const float y = kRockSpawnHeight + kRockHeightStep * static_cast<float>(i);

		std::unique_ptr<FallingRock> rock = std::make_unique<FallingRock>();
		rock->Initialize({ rockTargetsX_[i], y, 0.0f }, stage_);
		hazards_.push_back(std::move(rock));
	}
	rockTargetsX_.clear();
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
	// 消えたもの（次の更新で取り除く）は描かない
	for (const std::unique_ptr<Hazard>& hazard : hazards_) {
		if (hazard->IsAlive()) {
			hazard->Draw();
		}
	}
}

void Boss::DrawEffect() const {
	if (isAttackMarkVisible_) {
		attackMark_.Draw();
	}
	for (int32_t i = 0; i < visibleRockMarkCount_; ++i) {
		rockMarks_[i].Draw();
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
		ImGui::Text("staggerCooldown: %.2f  rockTargets: %d", staggerCooldownTimer_, static_cast<int>(rockTargetsX_.size()));
		ImGui::Checkbox("AI enabled", &isAiEnabled_);
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
