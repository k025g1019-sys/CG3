#include "Game/Object/Boss.h"

#include "Engine/Math/Collision.h"
#include "Engine/Math/Matrix4x4.h"
#include "Engine/Rendering/DebugDraw.h"
#include "Game/Object/Bit.h"
#include "Game/Object/FallingRock.h"
#include "Game/Object/Player.h"
#include "Game/Object/Shockwave.h"
#include "Game/Sound/SoundEffects.h"
#include "Game/Stage/Stage.h"
#include "Game/Util/Easing.h"
#include "Game/Util/GlobalVariables.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numbers>
#include <string>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

// 体の色（状態ごと）
const Vector4 kBodyColor = { 0.45f, 0.5f, 0.6f, 1.0f };     // 通常
const Vector4 kSleepColor = { 0.3f, 0.33f, 0.45f, 1.0f };   // 眠っている
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
const Vector4 kCrushMarkColor = { 1.0f, 0.2f, 0.4f, 0.4f };   // 押しつぶしの落下位置
const float kCrushMarkMargin = 0.6f;                          // 押しつぶしの印を体の幅より広げる量
const float kMarkHeight = 0.06f;                              // 印を床から浮かせる高さ（床と重なってちらつかないように）
const float kRockMarkSize = 1.4f;                             // 落石の印の直径

// 開発用：ImGui で調整項目を絞り込むときの 1 件（GlobalVariables のグループ名と、項目名の先頭の文字列。空ならグループの全項目）
struct TuningEntry {
	const char* group;
	const char* prefix;
};

// 開発用：ImGui で選べる行動（並びは表示の順番）と、その行動に関係する調整項目
struct SelectableBehavior {
	Boss::Behavior behavior;
	const char* name;
	TuningEntry tunings[4];  // group が nullptr の所で終わり
};

const SelectableBehavior kSelectableBehaviors[] = {
	{ Boss::Behavior::kCharge, "charge", { { "Boss", "Charge" }, { "Boss", "Tired" }, { nullptr, nullptr }, { nullptr, nullptr } } },
	{ Boss::Behavior::kSlam, "slam", { { "Boss", "Slam" }, { "Shockwave", "" }, { "Boss", "Bit" }, { "Bit", "" } } },
	{ Boss::Behavior::kRockFall, "rockfall", { { "Boss", "Rock" }, { "FallingRock", "" }, { nullptr, nullptr }, { nullptr, nullptr } } },
	{ Boss::Behavior::kCrush, "crush", { { "Boss", "Crush" }, { "Boss", "ShotDown" }, { "Boss", "Rock" }, { "FallingRock", "" } } },
	{ Boss::Behavior::kTired, "tired", { { "Boss", "Tired" }, { nullptr, nullptr }, { nullptr, nullptr }, { nullptr, nullptr } } },
	{ Boss::Behavior::kStagger, "stagger", { { "Boss", "Stagger" }, { "Boss", "ShotDown" }, { nullptr, nullptr }, { nullptr, nullptr } } },
	{ Boss::Behavior::kIdle, "idle", { { "Boss", "Idle" }, { "Boss", "FirstIdle" }, { "Boss", "TurnDelay" }, { nullptr, nullptr } } },
};
const int32_t kSelectableBehaviorCount = static_cast<int32_t>(sizeof(kSelectableBehaviors) / sizeof(kSelectableBehaviors[0]));
const int32_t kMaxTuningEntries = 4;  // SelectableBehavior::tunings の数

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
	globalVariables->AddItem(groupName, "EdgeMargin", kEdgeMargin);
	globalVariables->AddItem(groupName, "FirstIdleDuration", kFirstIdleDuration);
	globalVariables->AddItem(groupName, "IdleDuration", kIdleDuration);
	globalVariables->AddItem(groupName, "TurnDelay", kTurnDelay);
	globalVariables->AddItem(groupName, "ChargeWindup", kChargeWindup);
	globalVariables->AddItem(groupName, "ChargeSpeed", kChargeSpeed);
	globalVariables->AddItem(groupName, "ChargeDistance", kChargeDistance);
	globalVariables->AddItem(groupName, "TiredDuration", kTiredDuration);
	globalVariables->AddItem(groupName, "SlamWindup", kSlamWindup);
	globalVariables->AddItem(groupName, "SlamRecover", kSlamRecover);
	globalVariables->AddItem(groupName, "SlamRange", kSlamRange);
	globalVariables->AddItem(groupName, "RockRoar", kRockRoar);
	globalVariables->AddItem(groupName, "RockRecover", kRockRecover);
	globalVariables->AddItem(groupName, "RockSpawnHeight", kRockSpawnHeight);
	globalVariables->AddItem(groupName, "RockHeightStep", kRockHeightStep);
	globalVariables->AddItem(groupName, "RockSpreadX", kRockSpreadX);
	globalVariables->AddItem(groupName, "CrushCrouch", kCrushCrouch);
	globalVariables->AddItem(groupName, "CrushRiseDuration", kCrushRiseDuration);
	globalVariables->AddItem(groupName, "CrushRiseMaxDistance", kCrushRiseMaxDistance);
	globalVariables->AddItem(groupName, "CrushHoverHeight", kCrushHoverHeight);
	globalVariables->AddItem(groupName, "CrushHoverDuration", kCrushHoverDuration);
	globalVariables->AddItem(groupName, "CrushTrackSpeed", kCrushTrackSpeed);
	globalVariables->AddItem(groupName, "CrushLockDuration", kCrushLockDuration);
	globalVariables->AddItem(groupName, "CrushDropAcceleration", kCrushDropAcceleration);
	globalVariables->AddItem(groupName, "CrushDropMaxSpeed", kCrushDropMaxSpeed);
	globalVariables->AddItem(groupName, "CrushRecover", kCrushRecover);
	globalVariables->AddItem(groupName, "CrushRockDelay", kCrushRockDelay);
	globalVariables->AddItem(groupName, "ShotDownFallAcceleration", kShotDownFallAcceleration);
	globalVariables->AddItem(groupName, "ShotDownFallMaxSpeed", kShotDownFallMaxSpeed);
	globalVariables->AddItem(groupName, "ShotDownTiltSpeed", kShotDownTiltSpeed);
	globalVariables->AddItem(groupName, "ShotDownMaxTilt", kShotDownMaxTilt);
	globalVariables->AddItem(groupName, "HitStopDuration", kHitStopDuration);
	globalVariables->AddItem(groupName, "CoreHitStopDuration", kCoreHitStopDuration);
	globalVariables->AddItem(groupName, "HitShakeAmplitude", kHitShakeAmplitude);
	globalVariables->AddItem(groupName, "HitStopResistHitCount", kHitStopResistHitCount);
	globalVariables->AddItem(groupName, "HitStopResistResetTime", kHitStopResistResetTime);
	globalVariables->AddItem(groupName, "BoomerangHitInterval", kBoomerangHitInterval);
	globalVariables->AddItem(groupName, "StaggerDuration", kStaggerDuration);
	globalVariables->AddItem(groupName, "StaggerCooldown", kStaggerCooldown);
	globalVariables->AddItem(groupName, "DeathDuration", kDeathDuration);
	globalVariables->AddItem(groupName, "DeathSinkSpeed", kDeathSinkSpeed);
	globalVariables->AddItem(groupName, "FlashDuration", kFlashDuration);
	globalVariables->AddItem(groupName, "ShieldFlashDuration", kShieldFlashDuration);
	globalVariables->AddItem(groupName, "BitAnchorOffsetY", kBitAnchorOffsetY);
	globalVariables->AddItem(groupName, "SleepX", kSleepX);
	globalVariables->AddItem(groupName, "SleepHeight", kSleepHeight);
	globalVariables->AddItem(groupName, "SleepShieldHalfWidth", kSleepShieldHalfWidth);
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
	kEdgeMargin = globalVariables->GetFloatValue(groupName, "EdgeMargin");
	kFirstIdleDuration = globalVariables->GetFloatValue(groupName, "FirstIdleDuration");
	kIdleDuration = globalVariables->GetFloatValue(groupName, "IdleDuration");
	kTurnDelay = globalVariables->GetFloatValue(groupName, "TurnDelay");
	kChargeWindup = globalVariables->GetFloatValue(groupName, "ChargeWindup");
	kChargeSpeed = globalVariables->GetFloatValue(groupName, "ChargeSpeed");
	kChargeDistance = globalVariables->GetFloatValue(groupName, "ChargeDistance");
	kTiredDuration = globalVariables->GetFloatValue(groupName, "TiredDuration");
	kSlamWindup = globalVariables->GetFloatValue(groupName, "SlamWindup");
	kSlamRecover = globalVariables->GetFloatValue(groupName, "SlamRecover");
	kSlamRange = globalVariables->GetFloatValue(groupName, "SlamRange");
	kRockRoar = globalVariables->GetFloatValue(groupName, "RockRoar");
	kRockRecover = globalVariables->GetFloatValue(groupName, "RockRecover");
	kRockSpawnHeight = globalVariables->GetFloatValue(groupName, "RockSpawnHeight");
	kRockHeightStep = globalVariables->GetFloatValue(groupName, "RockHeightStep");
	kRockSpreadX = globalVariables->GetFloatValue(groupName, "RockSpreadX");
	kCrushCrouch = globalVariables->GetFloatValue(groupName, "CrushCrouch");
	kCrushRiseDuration = globalVariables->GetFloatValue(groupName, "CrushRiseDuration");
	kCrushRiseMaxDistance = globalVariables->GetFloatValue(groupName, "CrushRiseMaxDistance");
	kCrushHoverHeight = globalVariables->GetFloatValue(groupName, "CrushHoverHeight");
	kCrushHoverDuration = globalVariables->GetFloatValue(groupName, "CrushHoverDuration");
	kCrushTrackSpeed = globalVariables->GetFloatValue(groupName, "CrushTrackSpeed");
	kCrushLockDuration = globalVariables->GetFloatValue(groupName, "CrushLockDuration");
	kCrushDropAcceleration = globalVariables->GetFloatValue(groupName, "CrushDropAcceleration");
	kCrushDropMaxSpeed = globalVariables->GetFloatValue(groupName, "CrushDropMaxSpeed");
	kCrushRecover = globalVariables->GetFloatValue(groupName, "CrushRecover");
	kCrushRockDelay = globalVariables->GetFloatValue(groupName, "CrushRockDelay");
	kShotDownFallAcceleration = globalVariables->GetFloatValue(groupName, "ShotDownFallAcceleration");
	kShotDownFallMaxSpeed = globalVariables->GetFloatValue(groupName, "ShotDownFallMaxSpeed");
	kShotDownTiltSpeed = globalVariables->GetFloatValue(groupName, "ShotDownTiltSpeed");
	kShotDownMaxTilt = globalVariables->GetFloatValue(groupName, "ShotDownMaxTilt");
	kHitStopDuration = globalVariables->GetFloatValue(groupName, "HitStopDuration");
	kCoreHitStopDuration = globalVariables->GetFloatValue(groupName, "CoreHitStopDuration");
	kHitShakeAmplitude = globalVariables->GetFloatValue(groupName, "HitShakeAmplitude");
	kHitStopResistHitCount = globalVariables->GetIntValue(groupName, "HitStopResistHitCount");
	kHitStopResistResetTime = globalVariables->GetFloatValue(groupName, "HitStopResistResetTime");
	kBoomerangHitInterval = globalVariables->GetFloatValue(groupName, "BoomerangHitInterval");
	kStaggerDuration = globalVariables->GetFloatValue(groupName, "StaggerDuration");
	kStaggerCooldown = globalVariables->GetFloatValue(groupName, "StaggerCooldown");
	kDeathDuration = globalVariables->GetFloatValue(groupName, "DeathDuration");
	kDeathSinkSpeed = globalVariables->GetFloatValue(groupName, "DeathSinkSpeed");
	kFlashDuration = globalVariables->GetFloatValue(groupName, "FlashDuration");
	kShieldFlashDuration = globalVariables->GetFloatValue(groupName, "ShieldFlashDuration");
	kBitAnchorOffsetY = globalVariables->GetFloatValue(groupName, "BitAnchorOffsetY");
	kSleepX = globalVariables->GetFloatValue(groupName, "SleepX");
	kSleepHeight = globalVariables->GetFloatValue(groupName, "SleepHeight");
	kSleepShieldHalfWidth = globalVariables->GetFloatValue(groupName, "SleepShieldHalfWidth");
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

	std::random_device seed;
	randomEngine_.seed(seed());

	isAiEnabled_ = true;
	hazards_.clear();

	Reset();
}

void Boss::Reset() {
	isAttackMarkVisible_ = false;
	visibleRockMarkCount_ = 0;
	rockTargetsX_.clear();

	// ステージの右寄りに置き、プレイヤーのいる左を向く
	position_ = { stage_->GetRightEdge() - 8.0f, GetStandY(), 0.0f };
	facing_ = -1.0f;

	hp_ = kMaxHp;
	phase_ = 1;
	isDeathFinished_ = false;
	flashTimer_ = 0.0f;
	shieldFlashTimer_ = 0.0f;
	staggerCooldownTimer_ = 0.0f;
	facePlayerRequest_ = false;
	attackCycle_ = 0;
	fallSpeed_ = 0.0f;
	isShotDown_ = false;
	isWakingUp_ = false;
	bodyTilt_ = 0.0f;
	landing_ = Landing::kNone;
	isAttackEnabled_ = true;
	hitStop_.Stop();
	isHitStopFreezing_ = true;
	meleeHitCount_ = 0;
	meleeHitTimer_ = 0.0f;
	boomerangHitTimer_ = kBoomerangHitInterval;  // 最初の一撃はすぐ当たるように

	// 出ている攻撃物は消す（その場では壊さず、次の更新で取り除く。前のフレームに描いたものの定数バッファを壊さないため）
	KillAllHazards();

	// 最初だけ長めに待つ
	behaviorRequest_.reset();
	StartBehavior(Behavior::kIdle);
	idleDuration_ = kFirstIdleDuration;

	UpdateModels();
	UpdateMarks();
}

void Boss::Sleep() {
	fallSpeed_ = 0.0f;
	isWakingUp_ = false;

	behaviorRequest_.reset();
	StartBehavior(Behavior::kSleep);

	// 開始時の画面に入る位置に浮かせる（体の下端が床から SleepHeight の高さ）。起きたらこの真下へ落ちる
	const float stageCenterX = (stage_->GetLeftEdge() + stage_->GetRightEdge()) * 0.5f;
	position_.x = stageCenterX + kSleepX;
	position_.y = GetStandY() + kSleepHeight;

	UpdateModels();
	UpdateMarks();
}

#pragma endregion

#pragma region 更新

void Boss::Update() {
	// 床へ着いたかはフレームごとの出来事なので、毎回いったん戻す
	landing_ = Landing::kNone;

	// 近接攻撃を受けてからの時間を数え、間が空いたら「続けて受けた回数」を 0 に戻す（ヒットストップへの耐性が消える）
	meleeHitTimer_ += kFrameTime;
	if (meleeHitTimer_ > kHitStopResistResetTime) {
		meleeHitCount_ = 0;
	}

	// ブーメランでダメージを受けてからの時間（同じ相手へ続けて当たりすぎないよう、ApplyAttack で間隔を見る）
	boomerangHitTimer_ += kFrameTime;

	// ヒットストップ中は自分の動きだけ止める（行動の時間も進めないので、攻撃のタイミングが少し遅れる。行動は打ち切らない）。
	// 出してある攻撃物は動き続け、見た目は毎フレーム更新して揺らす。怯みややられへの切り替えは、止まり終わってから行う。
	// ヒットストップに耐性があるときは、揺れるだけで行動はそのまま進める
	if (hitStop_.IsActive()) {
		hitStop_.Update();
		if (isHitStopFreezing_) {
			UpdateHazards();
			UpdateModels();
			UpdateMarks();
			return;
		}
	}

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
	case Behavior::kSleep:
		UpdateSleep();
		break;
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
	case Behavior::kCrush:
		UpdateCrush();
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

	// 宙にいたら床まで落とす（押しつぶしの途中で怯んだ・倒されたとき、開発用に宙から行動を切り替えたとき）。
	// 押しつぶしで跳び上がってから落ちるまでは、UpdateCrush が高さを動かすので落とさない。眠っている間も浮いたままにする
	if (!IsCrushAirborne() && behavior_ != Behavior::kSleep) {
		UpdateFall();
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
		if (behavior_ == Behavior::kSleep) {
			DebugDraw::DrawAABB(GetSleepShieldBox(), { 0.6f, 0.85f, 1.0f, 1.0f });
		}
	}
#endif
}

void Boss::StartBehavior(Behavior next) {
	// 押しつぶしで真下へ落ちている最中だったか（やられたときの落ち方を決めるのに使う）
	const bool wasCrushDropping = (behavior_ == Behavior::kCrush) && (crushStep_ == CrushStep::kDrop);

	behavior_ = next;
	timer_ = 0.0f;

	// 打ち落とされた状態（ゆっくり落ちる・傾き）は、怯みとやられの間だけ続ける
	if (next != Behavior::kStagger && next != Behavior::kDead) {
		isShotDown_ = false;
		bodyTilt_ = 0.0f;
	}

	switch (next) {
	case Behavior::kIdle:
		idleDuration_ = kIdleDuration;
		turnTimer_ = 0.0f;
		break;
	case Behavior::kCharge:
		chargeStep_ = ChargeStep::kWindup;
		chargeRemaining_ = (phase_ >= 3) ? 2 : 1;  // 第 3 段階は 2 連続
		SoundEffects::Play(SoundEffects::Id::kChargeWindup);
		break;
	case Behavior::kSlam:
		slamStep_ = SlamStep::kWindup;
		SoundEffects::Play(SoundEffects::Id::kSlamWindup);
		break;
	case Behavior::kRockFall:
		rockFallStep_ = RockFallStep::kRoar;
		PlanRocks();  // 落下位置を先に決め、吠えている間は印を出す
		SoundEffects::Play(SoundEffects::Id::kRockFallWindup);
		break;
	case Behavior::kCrush:
		crushStep_ = CrushStep::kCrouch;
		fallSpeed_ = 0.0f;
		rockTargetsX_.clear();
		break;
	case Behavior::kDead:
		KillAllHazards();
		SoundEffects::Play(SoundEffects::Id::kBossDefeat);
		// 宙で倒されたら、打ち落とされたときと同じようにゆっくり落とす（押しつぶしで真下へ落ちている最中なら、急に遅くならないようそのまま落とす）
		if (position_.y > GetStandY() && !wasCrushDropping && !isShotDown_) {
			isShotDown_ = true;
			fallSpeed_ = 0.0f;
		}
		break;
	case Behavior::kTired:
	case Behavior::kStagger:
	default:
		break;
	}
}

void Boss::UpdateSleep() {
	// 浮いたまま動かない（起こされるのを待つ）
	timer_ += kFrameTime;
}

void Boss::WakeUp() {
	// 白く光らせ、次の更新で待機に切り替える（床へは UpdateFall で落ちる）。着いてから最初の攻撃までを待つ
	flashTimer_ = kFlashDuration;
	isWakingUp_ = true;
	behaviorRequest_ = Behavior::kIdle;
	facePlayerRequest_ = true;
}

void Boss::UpdateIdle() {
	timer_ += kFrameTime;
	UpdateTurnToPlayer();

	// プレイヤーがやられた後は、新しい攻撃を始めない（待機のまま）
	if (isAiEnabled_ && isAttackEnabled_ && timer_ >= idleDuration_) {
		ChooseNextAttack();
	}

	// 開発用：AI が無効で「繰り返す」にしているときは、待ち時間が過ぎたら選んでいる行動をもう一度出す
	if (!isAiEnabled_ && isLoopSelected_ && isAttackEnabled_ && timer_ >= idleDuration_) {
		const Behavior selected = GetSelectedBehavior();
		if (selected != Behavior::kIdle) {
			ForceBehavior(selected);
		}
	}
}

void Boss::ChooseNextAttack() {
	// 突進 → 叩きつけ → 落石 → 押しつぶし の順に出す（距離による差し替えはしない。叩きつけの衝撃波は遠くまで届く）
	Behavior next = Behavior::kCharge;
	switch (attackCycle_ % kAttackPatternCount) {
	case 0:
		next = Behavior::kCharge;
		break;
	case 1:
		next = Behavior::kSlam;
		break;
	case 2:
		next = Behavior::kRockFall;
		break;
	default:
		next = Behavior::kCrush;
		break;
	}
	attackCycle_++;

	// 開発用：攻撃を固定しているときは、順番や距離に関係なくその攻撃を出す
	switch (fixedAttackIndex_) {
	case 1:
		next = Behavior::kCharge;
		break;
	case 2:
		next = Behavior::kSlam;
		break;
	case 3:
		next = Behavior::kRockFall;
		break;
	case 4:
		next = Behavior::kCrush;
		break;
	default:
		break;
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
				// 2 連続：向き直ってもう一度（予告の音も鳴らし直す）
				FacePlayer();
				chargeStep_ = ChargeStep::kWindup;
				timer_ = 0.0f;
				SoundEffects::Play(SoundEffects::Id::kChargeWindup);
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
			// 第 2 段階（HP 半分以下）では、叩きつけのあとに浮遊ビットを呼ぶ（まだ残っているうちは呼ばない）
			if (IsEnraged() && !HasAliveBits()) {
				SpawnBits();
			}
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

void Boss::UpdateCrush() {
	timer_ += kFrameTime;

	// 浮いているときの体の中心の高さ
	const float hoverY = stage_->GetGroundY() + kCrushHoverHeight + kBodyHalfHeight;

	switch (crushStep_) {
	case CrushStep::kCrouch:
		// しゃがんで溜めたら、その時点のプレイヤーの頭上を目指して跳ぶ
		if (timer_ >= kCrushCrouch) {
			crushStartPosition_ = position_;

			// 跳び上がる動作で横に進める距離には上限がある（プレイヤーが遠いと、頭上までは届かず手前で浮く）
			float targetX = player_->GetPosition().x;
			if (targetX > position_.x + kCrushRiseMaxDistance) {
				targetX = position_.x + kCrushRiseMaxDistance;
			} else if (targetX < position_.x - kCrushRiseMaxDistance) {
				targetX = position_.x - kCrushRiseMaxDistance;
			}
			crushTargetX_ = std::clamp(targetX, GetMinX(), GetMaxX());
			crushStep_ = CrushStep::kRise;
			timer_ = 0.0f;

			// 跳び上がる瞬間の音
			SoundEffects::Play(SoundEffects::Id::kCrushJump);
		}
		break;

	case CrushStep::kRise: {
		// 終わりにかけて減速しながら、頭上の高さまで跳び上がる
		const float t = std::clamp(timer_ / kCrushRiseDuration, 0.0f, 1.0f);
		const float eased = EaseOutQuad(t);
		position_.x = Lerp(crushStartPosition_.x, crushTargetX_, eased);
		position_.y = Lerp(crushStartPosition_.y, hoverY, eased);
		if (t >= 1.0f) {
			crushStep_ = CrushStep::kHover;
			timer_ = 0.0f;
		}
		break;
	}

	case CrushStep::kHover: {
		// 浮いたまま、プレイヤーの頭上へ少しずつ寄っていく（走って逃げれば引き離せる速さ）
		const float toPlayer = player_->GetPosition().x - position_.x;
		if (std::fabs(toPlayer) <= kCrushTrackSpeed) {
			position_.x = player_->GetPosition().x;
		} else {
			position_.x += (toPlayer > 0.0f) ? kCrushTrackSpeed : -kCrushTrackSpeed;
		}
		position_.x = std::clamp(position_.x, GetMinX(), GetMaxX());
		position_.y = hoverY;
		if (timer_ >= kCrushHoverDuration) {
			crushStep_ = CrushStep::kLock;
			timer_ = 0.0f;
		}
		break;
	}

	case CrushStep::kLock:
		// 位置を決めて溜める（ここから先は追いかけない。落ちる直前の合図）
		if (timer_ >= kCrushLockDuration) {
			crushStep_ = CrushStep::kDrop;
			timer_ = 0.0f;
			fallSpeed_ = 0.0f;
		}
		break;

	case CrushStep::kDrop:
		// 加速しながら真下へ落ちる。床に着いたら余韻へ
		fallSpeed_ += kCrushDropAcceleration;
		if (fallSpeed_ > kCrushDropMaxSpeed) {
			fallSpeed_ = kCrushDropMaxSpeed;
		}
		position_.y -= fallSpeed_;
		if (position_.y <= GetStandY()) {
			position_.y = GetStandY();
			fallSpeed_ = 0.0f;
			crushStep_ = CrushStep::kRecover;
			timer_ = 0.0f;
			landing_ = Landing::kCrush;

			// 第 2 段階（HP 半分以下）では、着地のあとに岩を降らせる。落下位置をここで決め、少しの間は印を出す
			if (IsEnraged()) {
				PlanRocks();
			}
		}
		break;

	case CrushStep::kRecover: {
		// 印を出してから少しして岩を降らせる（待つ時間を余韻より長くしても、余韻の終わりには降らせる）
		const bool isRecoverEnd = timer_ >= kCrushRecover;
		if (!rockTargetsX_.empty() && (timer_ >= kCrushRockDelay || isRecoverEnd)) {
			SpawnRocks();
		}
		if (isRecoverEnd) {
			behaviorRequest_ = Behavior::kIdle;
		}
		break;
	}

	default:
		break;
	}
}

void Boss::UpdateFall() {
	// 床に立っている（または、やられて沈んでいる）ときは落とさない
	const float standY = GetStandY();
	if (position_.y <= standY) {
		// 床の上では打ち落とされた状態を残さない（残ると着地の処理を通らず、怯みが終わらなくなる）
		isShotDown_ = false;
		return;
	}

	// 加速しながら落ち、床に着いたら止まる。打ち落とされたときは、ゆっくり落とす
	const float acceleration = isShotDown_ ? kShotDownFallAcceleration : kCrushDropAcceleration;
	const float maxSpeed = isShotDown_ ? kShotDownFallMaxSpeed : kCrushDropMaxSpeed;
	fallSpeed_ += acceleration;
	if (fallSpeed_ > maxSpeed) {
		fallSpeed_ = maxSpeed;
	}
	position_.y -= fallSpeed_;
	if (position_.y <= standY) {
		position_.y = standY;
		fallSpeed_ = 0.0f;

		// 打ち落とされて着いた（ここから怯みの時間を数える）。宙で倒されて着いたときも同じ揺れと音にする
		if (isShotDown_ || behavior_ == Behavior::kDead) {
			isShotDown_ = false;
			landing_ = Landing::kShotDown;
		}

		// 起こされて着いた（ここから最初の攻撃までの待ちを数える）
		if (isWakingUp_) {
			isWakingUp_ = false;
			landing_ = Landing::kWakeUp;
			timer_ = 0.0f;
			idleDuration_ = kFirstIdleDuration;
		}
	}
}

void Boss::UpdateStagger() {
	// 打ち落とされて落ちている間は、体を前のめりに傾けていく。怯みの時間は床に着いてから数える
	if (isShotDown_) {
		bodyTilt_ += kShotDownTiltSpeed;
		if (bodyTilt_ > kShotDownMaxTilt) {
			bodyTilt_ = kShotDownMaxTilt;
		}
		return;
	}

	timer_ += kFrameTime;

	// 傾いていたら、怯んでいる間に起き上がる（怯みが終わるころに真っすぐに戻る）
	if (bodyTilt_ > 0.0f && kStaggerDuration > 0.0f) {
		bodyTilt_ -= kShotDownMaxTilt * kFrameTime / kStaggerDuration;
		if (bodyTilt_ < 0.0f) {
			bodyTilt_ = 0.0f;
		}
	}

	if (timer_ >= kStaggerDuration) {
		// 立ち直ったらすぐプレイヤーの方を向く（背中を殴られ続けないように）
		FacePlayer();
		behaviorRequest_ = Behavior::kIdle;
	}
}

void Boss::UpdateDead() {
	// 宙で倒されたときは、打ち落とされたときと同じように前のめりに傾けながら落ちる（落とすのは UpdateFall）。
	// 傾ききった落下の途中から倒れる動きを始め（傾きに続けて回すのでなめらかにつながる）、沈むのは床に着いてから
	if (position_.y > GetStandY()) {
		if (bodyTilt_ < kShotDownMaxTilt) {
			bodyTilt_ += kShotDownTiltSpeed;
			if (bodyTilt_ > kShotDownMaxTilt) {
				bodyTilt_ = kShotDownMaxTilt;
			}
			return;
		}
		timer_ += kFrameTime;
		return;
	}

	timer_ += kFrameTime;

	// 沈みながら倒れる
	position_.y -= kDeathSinkSpeed;
	if (timer_ >= kDeathDuration) {
		isDeathFinished_ = true;
	}
}

void Boss::StopAttacking() {
	isAttackEnabled_ = false;

	// このフレームに選んで次の更新で始まるはずだった攻撃も取り消す（怯みややられの予約はそのまま）
	if (behaviorRequest_ && IsAttackBehavior(*behaviorRequest_)) {
		behaviorRequest_.reset();
	}
}

bool Boss::IsAttackBehavior(Behavior behavior) {
	switch (behavior) {
	case Behavior::kCharge:
	case Behavior::kSlam:
	case Behavior::kRockFall:
	case Behavior::kCrush:
		return true;
	default:
		return false;
	}
}

void Boss::ForceBehavior(Behavior next) {
	// やられた後は切り替えない
	if (!IsAlive()) {
		return;
	}

	// 攻撃はプレイヤーの方を向いてから始める（AI が選んだときと同じ）
	if (IsAttackBehavior(next)) {
		FacePlayer();
	}
	behaviorRequest_ = next;
}

Boss::Behavior Boss::GetSelectedBehavior() const {
	if (selectedBehaviorIndex_ < 0 || selectedBehaviorIndex_ >= kSelectableBehaviorCount) {
		return Behavior::kIdle;
	}
	return kSelectableBehaviors[selectedBehaviorIndex_].behavior;
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
	// HP が半分以下で第 2 段階（叩きつけのあとのビット、押しつぶしの着地のあとの落石が加わる）
	if (phase_ == 1 && hp_ * 2 <= kMaxHp) {
		phase_ = 2;
	}
	// 1/3 以下で第 3 段階（突進が 2 連続）
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
	case Behavior::kSleep:
		color = kSleepColor;
		break;
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
	case Behavior::kCrush:
		if (crushStep_ == CrushStep::kCrouch) {
			// しゃがんで溜める（足元を床に付けたまま縮む）
			body.scale.y *= 0.8f;
			body.translate.y -= kBodyHalfHeight * 0.2f;
			color = kWindupColor;
		} else if (crushStep_ == CrushStep::kLock) {
			// 落ちる直前に震える
			body.translate.x += std::sin(timer_ * 60.0f) * 0.08f;
			color = kWindupColor;
		} else if (crushStep_ == CrushStep::kDrop) {
			color = kWindupColor;
		}
		break;
	case Behavior::kTired:
		color = kTiredColor;
		break;
	case Behavior::kStagger:
		// 打ち落とされたときは前のめりに傾く（着地後の怯みの間に起き上がる）
		body.rotate.z = CalcTiltRotateZ();
		color = kStaggerColor;
		break;
	case Behavior::kDead:
		// 倒れていく（打ち落とされて傾いていたら、その傾きから倒れる）
		body.rotate.z = CalcTiltRotateZ() - facing_ * (timer_ / kDeathDuration) * 0.8f;
		color = kDeadColor;
		break;
	case Behavior::kIdle:
	default:
		break;
	}
	if (flashTimer_ > 0.0f) {
		color = kFlashColor;
	}

	// ヒットストップ中は見た目だけ揺らす（体・盾・コアを同じだけずらす。判定の位置は動かさない）
	const Vector3 shake = hitStop_.GetShakeOffset();
	body.translate += shake;
	body_.SetColor(color);
	body_.Update();

	// --- 盾（正面。眠っている間は体の下に横向きに構える）。弾いた直後は白く光る ---
	Transform3D& shield = shield_.GetTransform();
	if (behavior_ == Behavior::kSleep) {
		shield.scale = { kSleepShieldHalfWidth * 2.0f, kSleepShieldHalfThickness * 2.0f, kBodyHalfWidth * 2.0f + 0.2f };
	} else {
		shield.scale = { 0.3f, kBodyHalfHeight * 2.0f + 0.2f, kBodyHalfWidth * 2.0f + 0.2f };
	}
	shield.translate = GetShieldPosition() + shake;
	shield_.SetColor((shieldFlashTimer_ > 0.0f) ? kFlashColor : kShieldColor);
	shield_.Update();

	// --- コア（背中）。少し脈打たせて目立たせる ---
	const Sphere core = GetCoreSphere();
	const float pulse = 1.0f + 0.1f * std::sin(timer_ * 6.0f);
	Transform3D& coreTransform = core_.GetTransform();
	coreTransform.translate = core.center + shake;
	coreTransform.scale = { core.radius * 2.0f * pulse, core.radius * 2.0f * pulse, core.radius * 2.0f * pulse };
	core_.Update();
}

void Boss::UpdateMarks() {
	const float groundY = stage_->GetGroundY();

	// --- 突進・叩きつけ・押しつぶしの予備動作：これから攻撃が来る範囲を床の上の半透明の帯で示す ---
	isAttackMarkVisible_ = false;
	float length = 0.0f;
	float centerX = position_.x;
	float progress = 0.0f;
	Vector4 color = kChargeMarkColor;
	if (behavior_ == Behavior::kCharge && chargeStep_ == ChargeStep::kWindup) {
		// 走る距離（壁までの方が短ければそちら）。体の正面から先へ伸ばす
		const float toWall = (facing_ > 0.0f) ? (GetMaxX() - position_.x) : (position_.x - GetMinX());
		length = (toWall < kChargeDistance) ? toWall : kChargeDistance;
		centerX = position_.x + facing_ * (kBodyHalfWidth + length * 0.5f);
		progress = timer_ / kChargeWindup;
		isAttackMarkVisible_ = true;
	} else if (behavior_ == Behavior::kSlam && slamStep_ == SlamStep::kWindup) {
		length = kSlamRange;
		centerX = position_.x + facing_ * (kBodyHalfWidth + length * 0.5f);
		progress = timer_ / kSlamWindup;
		color = kSlamMarkColor;
		isAttackMarkVisible_ = true;
	} else if (behavior_ == Behavior::kCrush && crushStep_ != CrushStep::kCrouch && crushStep_ != CrushStep::kRecover) {
		// 押しつぶし：落ちてくる位置（体の真下）。浮いて追いかけている間は薄く、止まって溜めたら濃くする
		length = kBodyHalfWidth * 2.0f + kCrushMarkMargin;
		centerX = position_.x;
		progress = (crushStep_ == CrushStep::kLock || crushStep_ == CrushStep::kDrop) ? 1.0f : 0.3f;
		color = kCrushMarkColor;
		isAttackMarkVisible_ = true;
	}
	if (isAttackMarkVisible_ && length > 0.0f) {
		// 予備動作が進むほど濃くする
		color.w = 0.15f + 0.35f * std::clamp(progress, 0.0f, 1.0f);
		Transform3D& mark = attackMark_.GetTransform();
		mark.translate = { centerX, groundY + kMarkHeight, 0.0f };
		mark.scale = { length, kMarkHeight * 2.0f, kBodyHalfWidth * 2.0f };
		attackMark_.SetColor(color);
		attackMark_.Update();
	} else {
		isAttackMarkVisible_ = false;
	}

	// --- 落石の予備動作：落ちる位置に半透明の円盤を置き、点滅させる ---
	visibleRockMarkCount_ = 0;
	const bool isRockFallRoar = (behavior_ == Behavior::kRockFall) && (rockFallStep_ == RockFallStep::kRoar);
	const bool isCrushRockWait = (behavior_ == Behavior::kCrush) && (crushStep_ == CrushStep::kRecover) && !rockTargetsX_.empty();
	if (isRockFallRoar || isCrushRockWait) {
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
	// 止めて見せている間もヒットストップの揺れは進める（進めないと、揺れでずれた位置のまま固まって見える）
	hitStop_.Update();
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

Boss::HitResult Boss::ApplyAttack(const Sphere& attackSphere, int32_t damage, const Vector3& attackerPosition, AttackType type) {
	if (!IsAlive()) {
		return HitResult::kNone;
	}
	// 眠っている間は、この攻撃では起きない（盾や下からなら弾き、体かコアに当たったら身じろぎだけする）
	if (behavior_ == Behavior::kSleep) {
		return ApplySleepingAttack(attackSphere, attackerPosition, false);
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

	// ブーメランは、前にブーメランでダメージを受けてから一定の時間がたつまで当たらない（同じ相手へ続けて当たりすぎないように）。
	// 大技の一撃は間隔に関係なく当たり、そこから数え直す
	const bool isBoomerang = (type == AttackType::kBoomerang) || (type == AttackType::kFollowBoomerang);
	if (isBoomerang && boomerangHitTimer_ < kBoomerangHitInterval) {
		return HitResult::kNone;
	}
	if (type != AttackType::kMelee) {
		boomerangHitTimer_ = 0.0f;
	}
	const bool isMelee = (type == AttackType::kMelee);

	// 押しつぶしで宙に浮いている間は、追従を使っていない投げ（通常の投げと大技）が体かコアに当たれば打ち落とす
	// （下から当てると先に体に当たり、そのあと攻撃の間隔のあいだはコアに当たらないので、体でも打ち落とせるようにする）。
	// 追従を使った投げは打ち落とせない（ダメージだけ）
	const bool isCrushAirborne = (behavior_ == Behavior::kCrush) && (position_.y > GetStandY());
	const bool canShootDown = (type == AttackType::kBoomerang) || (type == AttackType::kBurst);

	// 弱点のコア：ダメージ 2 倍。怯むのは攻撃していないときと、押しつぶしで浮いている間だけ（ほかの攻撃は続ける）。
	// 怯みは間隔を空ける（連続で怯ませて封じられないように）
	if (touchesCore) {
		TakeDamage(damage * kWeakPointMultiplier);
		StartHitStop(kCoreHitStopDuration, isMelee);
		if (IsAlive()) {
			if (isCrushAirborne) {
				// 浮いている間は、打ち落とせる攻撃のときだけ打ち落とす
				if (canShootDown && CanStagger() && staggerCooldownTimer_ <= 0.0f) {
					ShootDown();
				}
			} else if (CanStagger() && staggerCooldownTimer_ <= 0.0f) {
				// 床の上なら、その場で怯む（押しつぶしの跳び上がる瞬間でまだ床にいるときも。床の上で打ち落とすと着地しないので、怯みが終わらない）
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
	StartHitStop(kHitStopDuration, isMelee);
	if (IsAlive() && behavior_ == Behavior::kIdle) {
		facePlayerRequest_ = true;
	}
	// 押しつぶしで浮いている間は、打ち落とせる攻撃なら体に当たっても打ち落とす
	if (IsAlive() && isCrushAirborne && canShootDown && CanStagger() && staggerCooldownTimer_ <= 0.0f) {
		ShootDown();
	}
	return HitResult::kDamaged;
}

Boss::HitResult Boss::ApplySleepingAttack(const Sphere& attackSphere, const Vector3& attackerPosition, bool canWakeUp) {
	if (behavior_ != Behavior::kSleep) {
		return HitResult::kNone;
	}
	const bool touchesShield = IsCollision(GetSleepShieldBox(), attackSphere);
	const bool touchesCore = IsCollision(attackSphere, GetCoreSphere());
	const bool touchesBody = IsCollision(GetBodyBox(), attackSphere);
	if (!touchesShield && !touchesCore && !touchesBody) {
		return HitResult::kNone;
	}

	// 体の下の盾で弾くのは、盾に当たっていて、体の下端より下から来たか、体にもコアにも当たっていないとき。
	// 体の側面に当たったときは、盾の端に触れていても弾かない（見た目どおり体に当たった扱いにする）
	const bool isFromBelow = attackerPosition.y < position_.y - kBodyHalfHeight;
	if (touchesShield && (isFromBelow || (!touchesBody && !touchesCore))) {
		shieldFlashTimer_ = kShieldFlashDuration;
		return HitResult::kBlocked;
	}

	// 起こせる攻撃（曲げて投げたブーメラン）なら起きる（ダメージは受けない）
	if (canWakeUp) {
		WakeUp();
		return HitResult::kWokenUp;
	}

	// 起こせない攻撃は、白く光って揺れる（身じろぎ）だけで眠ったまま。
	// 同じ攻撃が重なっている間に毎フレーム揺れ直さないよう、ブーメランのダメージと同じ間隔を空ける
	if (boomerangHitTimer_ < kBoomerangHitInterval) {
		return HitResult::kNone;
	}
	boomerangHitTimer_ = 0.0f;
	flashTimer_ = kFlashDuration;
	StartHitStop(kCoreHitStopDuration, false);
	return HitResult::kStirred;
}

void Boss::StartHitStop(float duration, bool isMelee) {
	// 近接攻撃を続けて受けた回数を数える（前の近接から間が空いていたら 1 から数え直す）
	if (isMelee) {
		if (meleeHitCount_ > 0 && meleeHitTimer_ <= kHitStopResistResetTime) {
			meleeHitCount_++;
		} else {
			meleeHitCount_ = 1;
		}
		meleeHitTimer_ = 0.0f;
	}

	// 耐性があるときは揺れるだけで止めない（攻撃のタイミングが遅れない）
	isHitStopFreezing_ = !IsHitStopResistant();
	hitStop_.Start(duration, kHitShakeAmplitude);
}

void Boss::ShootDown() {
	// 押しつぶしで浮いているところを打ち落とす（体を傾けながらゆっくり床へ落ち、着地してから怯む）
	isShotDown_ = true;
	fallSpeed_ = 0.0f;
	behaviorRequest_ = Behavior::kStagger;
	staggerCooldownTimer_ = kStaggerCooldown;
}

bool Boss::CanStagger() const {
	// 攻撃していないとき
	if (behavior_ == Behavior::kIdle || behavior_ == Behavior::kTired) {
		return true;
	}
	// 押しつぶしで浮いている間（跳び上がり〜溜め）。怯むとそのまま床へ落ちる
	if (behavior_ == Behavior::kCrush) {
		return crushStep_ == CrushStep::kRise || crushStep_ == CrushStep::kHover || crushStep_ == CrushStep::kLock;
	}
	return false;
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
	// 背中側の位置。打ち落とされて体が傾いているときは、体と一緒に回す
	Vector3 offset = { -facing_ * kCoreOffsetX, kCoreOffsetY, 0.0f };
	const float rotateZ = CalcTiltRotateZ();
	if (rotateZ != 0.0f) {
		const float c = std::cos(rotateZ);
		const float s = std::sin(rotateZ);
		offset = { offset.x * c - offset.y * s, offset.x * s + offset.y * c, 0.0f };
	}
	return { position_ + offset, kCoreRadius };
}

float Boss::CalcTiltRotateZ() const {
	// 体の正面の側へ倒れる向き（向きが左なら反時計回り）
	return -facing_ * bodyTilt_;
}

Vector3 Boss::GetShieldPosition() const {
	// 眠っている間は体の下
	if (behavior_ == Behavior::kSleep) {
		return { position_.x, position_.y - kBodyHalfHeight - kSleepShieldHalfThickness, 0.0f };
	}
	return { position_.x + facing_ * kShieldOffsetX, position_.y + 0.1f, 0.0f };
}

AABB3D Boss::GetSleepShieldBox() const {
	return MakeAABB(GetShieldPosition(), { kSleepShieldHalfWidth, kSleepShieldHalfThickness, kBodyHalfWidth + 0.1f });
}

bool Boss::IsGuarding() const {
	if (!IsAlive()) {
		return false;
	}
	return behavior_ != Behavior::kTired && behavior_ != Behavior::kStagger && behavior_ != Behavior::kDead;
}

bool Boss::IsBodyDamaging() const {
	const bool isDashing = (behavior_ == Behavior::kCharge) && (chargeStep_ == ChargeStep::kDash);
	const bool isDropping = (behavior_ == Behavior::kCrush) && (crushStep_ == CrushStep::kDrop);
	return isDashing || isDropping;
}

bool Boss::IsInFront(const Vector3& point) const {
	return (point.x - position_.x) * facing_ > 0.0f;
}

float Boss::CalcPlayerSide() const {
	return (player_->GetPosition().x >= position_.x) ? 1.0f : -1.0f;
}

float Boss::GetMinX() const {
	// 壁から少し離す（背中のコアは体からはみ出しているので、壁にくっつくとコアが壁に埋まってブーメランを当てにくい）
	return stage_->GetLeftEdge() + kBodyHalfWidth + kEdgeMargin;
}

float Boss::GetMaxX() const {
	return stage_->GetRightEdge() - kBodyHalfWidth - kEdgeMargin;
}

float Boss::GetStandY() const {
	return stage_->GetGroundY() + kBodyHalfHeight;
}

bool Boss::IsCrushAirborne() const {
	if (behavior_ != Behavior::kCrush) {
		return false;
	}
	return crushStep_ != CrushStep::kCrouch && crushStep_ != CrushStep::kRecover;
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

bool Boss::HasAliveBits() const {
	for (const std::unique_ptr<Hazard>& hazard : hazards_) {
		if (hazard->IsAlive() && hazard->IsBit()) {
			return true;
		}
	}
	return false;
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
	case Behavior::kSleep:
		return "sleep";
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
	case Behavior::kCrush:
		switch (crushStep_) {
		case CrushStep::kCrouch:
			return "crush(crouch)";
		case CrushStep::kRise:
			return "crush(rise)";
		case CrushStep::kHover:
			return "crush(hover)";
		case CrushStep::kLock:
			return "crush(lock)";
		case CrushStep::kDrop:
			return "crush(drop)";
		case CrushStep::kRecover:
		default:
			return "crush(recover)";
		}
	case Behavior::kStagger:
		return "stagger";
	case Behavior::kDead:
		return "dead";
	default:
		return "?";
	}
}

#ifdef USE_IMGUI
void Boss::DrawSelectedTuningImGui() {
	if (selectedBehaviorIndex_ < 0 || selectedBehaviorIndex_ >= kSelectableBehaviorCount) {
		return;
	}
	const SelectableBehavior& selected = kSelectableBehaviors[selectedBehaviorIndex_];
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();

	// 関係する調整項目を並べる（ボス以外のグループは見出しを付ける）
	for (int32_t i = 0; i < kMaxTuningEntries && selected.tunings[i].group != nullptr; ++i) {
		const TuningEntry& entry = selected.tunings[i];
		if (std::strcmp(entry.group, "Boss") != 0) {
			ImGui::TextDisabled("%s", entry.group);
		}
		globalVariables->DrawItemsImGui(entry.group, entry.prefix);
	}

	// 表示しているグループを CSV に保存する（同じグループは 1 回だけ）
	if (ImGui::Button("save shown groups")) {
		for (int32_t i = 0; i < kMaxTuningEntries && selected.tunings[i].group != nullptr; ++i) {
			bool isSaved = false;
			for (int32_t j = 0; j < i; ++j) {
				if (std::strcmp(selected.tunings[j].group, selected.tunings[i].group) == 0) {
					isSaved = true;
				}
			}
			if (!isSaved) {
				globalVariables->SaveFile(selected.tunings[i].group);
			}
		}
	}
}

void Boss::DrawImGui() {
	if (ImGui::TreeNode("Boss")) {
		ImGui::Text("hp: %d / %d  phase: %d  behavior: %s", hp_, kMaxHp, phase_, GetBehaviorName());
		ImGui::Text("position: %.2f, %.2f  facing: %s  guarding: %s  hazards: %d",
			position_.x, position_.y, (facing_ > 0.0f) ? "right" : "left", IsGuarding() ? "true" : "false", static_cast<int>(hazards_.size()));
		ImGui::Text("staggerCooldown: %.2f  rockTargets: %d  hitStop: %.2f%s  attackEnabled: %s",
			staggerCooldownTimer_, static_cast<int>(rockTargetsX_.size()), hitStop_.GetRemainingTime(),
			isHitStopFreezing_ ? "" : " (shake only)", isAttackEnabled_ ? "true" : "false");
		ImGui::Text("meleeHits: %d  hitStopResist: %s  enraged (phase 2+): %s  boomerangHit: %.2f",
			meleeHitCount_, IsHitStopResistant() ? "on" : "off", IsEnraged() ? "true" : "false", boomerangHitTimer_);
		ImGui::Checkbox("AI enabled", &isAiEnabled_);

		// AI が選ぶ攻撃を 1 つに固定する（同じ攻撃を繰り返し見たいとき。AI が有効な間だけ意味がある）
		const char* fixedAttackItems[] = { "cycle (charge > slam > rockfall > crush)", "charge", "slam", "rockfall", "crush" };
		ImGui::Combo("AI attack", &fixedAttackIndex_, fixedAttackItems, IM_ARRAYSIZE(fixedAttackItems));

		// 行動を選ぶ。選んだ行動を今すぐ出す・繰り返す（AI が無効のとき）、選んだ行動の調整項目だけを下に並べる
		ImGui::SeparatorText("behavior");
		const char* behaviorNames[kSelectableBehaviorCount];
		for (int32_t i = 0; i < kSelectableBehaviorCount; ++i) {
			behaviorNames[i] = kSelectableBehaviors[i].name;
		}
		ImGui::Combo("behavior", &selectedBehaviorIndex_, behaviorNames, kSelectableBehaviorCount);
		if (ImGui::Button("do now")) {
			ForceBehavior(GetSelectedBehavior());
		}
		ImGui::SameLine();
		ImGui::Checkbox("loop (when AI is off)", &isLoopSelected_);

		ImGui::SeparatorText("tuning (selected behavior)");
		DrawSelectedTuningImGui();
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
