#include "Game/Scene/BossScene.h"

#include "Engine/Input/Input.h"
#include "Engine/Math/Collision.h"
#include "Game/Effect/GuardEffect.h"
#include "Game/Effect/HitEffect.h"
#include "Game/Object/Bit.h"
#include "Game/Object/FallingRock.h"
#include "Game/Object/Shockwave.h"
#include "Game/Scene/ClearScene.h"
#include "Game/Scene/TitleScene.h"
#include "Game/Util/GlobalVariables.h"
#include "Game/Util/ScreenToWorld.h"

#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

const Vector4 kPlayerGaugeColor = { 0.3f, 0.9f, 0.4f, 1.0f };  // プレイヤーのライフの色
const Vector4 kBossGaugeColor = { 0.9f, 0.25f, 0.25f, 1.0f };  // ボスの HP の色

// ヒットエフェクトの色
const Vector4 kEnemyHitColor = { 1.0f, 0.95f, 0.6f, 1.0f };    // ボスの体に当たった
const Vector4 kCoreHitColor = { 1.0f, 0.6f, 0.2f, 1.0f };      // コアに当たった
const Vector4 kBreakColor = { 0.9f, 0.9f, 0.9f, 1.0f };        // 攻撃物を壊した
const Vector4 kPlayerHitColor = { 1.0f, 0.3f, 0.3f, 1.0f };    // プレイヤーが被弾した

} // namespace

#pragma region 調整値

void BossScene::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "BossScene";
	globalVariables->CreateGroup(groupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(groupName, "DeathFreezeDuration", kDeathFreezeDuration);

	// このシーンが持つオブジェクトの分
	Player::RegisterGlobalVariables();
	Boomerang::RegisterGlobalVariables();
	Boss::RegisterGlobalVariables();
	Shockwave::RegisterGlobalVariables();
	FallingRock::RegisterGlobalVariables();
	Bit::RegisterGlobalVariables();
	CameraController::RegisterGlobalVariables();
	Stage::RegisterGlobalVariables();
	DeathParticles::RegisterGlobalVariables();
	Gauge::RegisterGlobalVariables();
}

void BossScene::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "BossScene";
	kDeathFreezeDuration = globalVariables->GetFloatValue(groupName, "DeathFreezeDuration");

	// このシーンが持つオブジェクトの分
	Player::ApplyGlobalVariables();
	Boomerang::ApplyGlobalVariables();
	Boss::ApplyGlobalVariables();
	Shockwave::ApplyGlobalVariables();
	FallingRock::ApplyGlobalVariables();
	Bit::ApplyGlobalVariables();
	CameraController::ApplyGlobalVariables();
	Stage::ApplyGlobalVariables();
	DeathParticles::ApplyGlobalVariables();
	Gauge::ApplyGlobalVariables();
}

#pragma endregion

#pragma region 初期化

void BossScene::OnInitialize() {
	// 光は斜め上から当て、立体の面の向きが分かるようにする
	directionalLight_.direction = { 0.4f, -1.0f, 0.6f };

	stage_.Initialize();
	player_.Initialize(&stage_);
	boss_.Initialize(&stage_, &player_);

	// カメラはプレイヤーに追従させる。開始時は補間なしで位置を合わせる
	cameraController_.Initialize(&camera_, &player_, &stage_);
	cameraController_.Reset(GetAspectRatio());

	// UI（ゲージは満タンから始まる）
	playerGauge_.Initialize(kPlayerGaugePosition, kPlayerGaugeSize, kPlayerGaugeColor);
	bossGauge_.Initialize(kBossGaugePosition, kBossGaugeSize, kBossGaugeColor);

	// フェードインから始める
	fade_.Initialize();
	fade_.Start(Fade::Status::kFadeIn, kFadeDuration);

	phase_ = Phase::kPlay;
	deathTimer_ = 0.0f;
	deathParticles_.reset();
	effects_.clear();
}

#pragma endregion

#pragma region 更新

void BossScene::OnUpdate() {
	// 調整値を反映（ImGui での変更を毎フレームゲームへ反映する）
	ApplyGlobalVariables();

#ifndef NDEBUG
	// 開発用のショートカット：Esc でタイトル（即時）、C でクリアへ（フェード付き）、P でボスの AI を止める、H でプレイヤーにダメージ
	Input* input = Input::GetInstance();
	if (input->IsTrigger(DIK_ESCAPE)) {
		ChangeScene<TitleScene>();
	}
	if (input->IsTrigger(DIK_C) && phase_ == Phase::kPlay) {
		StartClear();
	}
	if (input->IsTrigger(DIK_P)) {
		boss_.SetAiEnabled(!boss_.IsAiEnabled());
	}
	if (input->IsTrigger(DIK_H) && phase_ == Phase::kPlay) {
		if (player_.OnDamage(1.0f)) {
			SpawnHitEffect(player_.GetPosition(), kPlayerHitColor);
		}
	}
#endif

	switch (phase_) {
	case Phase::kPlay:
		UpdatePlay();
		break;
	case Phase::kDeath:
		UpdateDeath();
		break;
	case Phase::kClear:
		UpdateClear();
		break;
	}

	// エフェクトはどの段階でも進める
	UpdateEffects();

	// カメラはプレイヤーが動いた後に追従させる。ボスが生きている間はボスの方へ少し寄せる
	if (boss_.IsAlive()) {
		cameraController_.SetFocusPosition(boss_.GetPosition());
	} else {
		cameraController_.ClearFocus();
	}
	cameraController_.Update(GetAspectRatio());

	// UI はライフと HP の割合を映す
	playerGauge_.Update(static_cast<float>(player_.GetLife()) / static_cast<float>(player_.GetMaxLife()));
	bossGauge_.Update(static_cast<float>(boss_.GetHp()) / static_cast<float>(boss_.GetMaxHp()));

	// フェード。フェードインが終わったら止める（戦闘中は最前面のスプライトを描かない）
	fade_.Update(GetRenderAreaWidth(), GetRenderAreaHeight());
	if (fade_.GetStatus() == Fade::Status::kFadeIn && fade_.IsFinished()) {
		fade_.Stop();
	}
}

void BossScene::UpdatePlay() {
	UpdateObjects();
	CheckAllCollisions();

	// ライフが尽きたらやられ演出へ
	if (player_.IsDead()) {
		StartDeath();
		return;
	}

	// ボスを倒してやられ演出が終わったらクリアへ
	if (boss_.IsDeathFinished()) {
		StartClear();
	}
}

void BossScene::UpdateDeath() {
	deathTimer_ += kFrameTime;

	// 止めて見せている間も見た目は毎フレーム更新する（Update を止めると描画が前のフレームの位置と交互になってガタつく）
	stage_.Update();
	player_.UpdateModel();
	boss_.UpdateModel();

	// 一瞬止まって見せてから、プレイヤーの位置にパーティクルを出す（プレイヤーは消す）
	if (deathTimer_ >= kDeathFreezeDuration && !deathParticles_) {
		deathParticles_ = std::make_unique<DeathParticles>();
		deathParticles_->Initialize(player_.GetPosition());
	}
	if (deathParticles_) {
		deathParticles_->Update();

		// パーティクルが消えたらフェードアウトを始める
		if (deathParticles_->IsFinished() && fade_.GetStatus() == Fade::Status::kNone) {
			fade_.Start(Fade::Status::kFadeOut, kFadeDuration);
		}
	}

	// フェードアウトが終わったらタイトルへ
	if (fade_.GetStatus() == Fade::Status::kFadeOut && fade_.IsFinished()) {
		ChangeScene<TitleScene>();
	}
}

void BossScene::UpdateClear() {
	// 倒した後も動かしておく（ボスはやられたまま、攻撃物は消えている）
	UpdateObjects();

	// フェードアウトが終わったらクリア画面へ
	if (fade_.GetStatus() == Fade::Status::kFadeOut && fade_.IsFinished()) {
		ChangeScene<ClearScene>();
	}
}

void BossScene::StartDeath() {
	phase_ = Phase::kDeath;
	deathTimer_ = 0.0f;
}

void BossScene::StartClear() {
	phase_ = Phase::kClear;
	fade_.Start(Fade::Status::kFadeOut, kFadeDuration);
}

void BossScene::UpdateObjects() {
	// カーソルの位置をワールド座標に直してプレイヤーへ渡してから更新する
	cursorWorldPosition_ = CalcCursorWorldPosition();
	player_.SetCursorWorldPosition(cursorWorldPosition_);

	stage_.Update();
	player_.Update();
	boss_.Update();
}

Vector3 BossScene::CalcCursorWorldPosition() const {
	// カーソルはウィンドウのクライアント座標なので、描画先の左上を原点にし直す
	const Vector2 mouse = Input::GetInstance()->GetMousePosition();
	const Vector2 local = { mouse.x - GetRenderAreaX(), mouse.y - GetRenderAreaY() };

	// 今のカメラのビュー射影で平面 Z=0 へ戻す
	const Matrix4x4 viewProjection = camera_.GetViewMatrix() * camera_.GetProjectionMatrix(GetAspectRatio());
	return ScreenToWorldOnGamePlane(local, GetRenderAreaWidth(), GetRenderAreaHeight(), viewProjection);
}

bool BossScene::IsPlayerVisible() const {
	// やられ演出では止まって見せている間だけ描き、パーティクルに置き換わったら消す
	if (phase_ == Phase::kDeath) {
		return deathTimer_ < kDeathFreezeDuration;
	}
	return true;
}

const char* BossScene::GetPhaseName() const {
	switch (phase_) {
	case Phase::kPlay:
		return "play";
	case Phase::kDeath:
		return "death";
	case Phase::kClear:
		return "clear";
	}
	return "unknown";
}

#pragma endregion

#pragma region 当たり判定

void BossScene::CheckAllCollisions() {
	Boomerang& boomerang = player_.GetBoomerang();

	// --- プレイヤーの近接攻撃 → ボス（1 回の攻撃で 1 ヒット）---
	if (player_.IsAttacking() && !player_.HasAttackHit()) {
		const Sphere attackSphere = player_.GetAttackSphere();
		const Boss::HitResult result = boss_.ApplyAttack(attackSphere, Player::kMeleeDamage, player_.GetPosition());
		if (result != Boss::HitResult::kNone) {
			player_.MarkAttackHit();
			SpawnBossHitEffect(result, attackSphere.center);
		}
	}

	// --- ブーメラン → ボス（往路・復路で 1 回ずつ。盾に当たれば弾かれて戻る）---
	if (boomerang.CanHit()) {
		const Sphere boomerangSphere = boomerang.GetHitSphere();
		const Boss::HitResult result = boss_.ApplyAttack(boomerangSphere, Boomerang::kDamage, boomerangSphere.center);
		if (result == Boss::HitResult::kBlocked) {
			boomerang.Repel();
		}
		if (result != Boss::HitResult::kNone) {
			boomerang.MarkHit();
			SpawnBossHitEffect(result, boomerangSphere.center);
		}
	}

	// --- プレイヤーの攻撃 → 壊せる攻撃物（岩・ビット）---
	for (const std::unique_ptr<Hazard>& hazard : boss_.GetHazards()) {
		if (!hazard->IsAlive() || !hazard->CanBeDestroyed()) {
			continue;
		}
		const Sphere hazardSphere = hazard->GetHitSphere();
		const bool hitByMelee = player_.IsAttacking() && IsCollision(player_.GetAttackSphere(), hazardSphere);
		const bool hitByBoomerang = boomerang.IsFlying() && IsCollision(boomerang.GetHitSphere(), hazardSphere);
		if (hitByMelee || hitByBoomerang) {
			hazard->OnHitByPlayer();
			SpawnHitEffect(hazardSphere.center, kBreakColor);
		}
	}

	// --- ボスの攻撃物 → プレイヤー ---
	const Sphere playerBody = player_.GetBodySphere();
	for (const std::unique_ptr<Hazard>& hazard : boss_.GetHazards()) {
		if (!hazard->IsAlive()) {
			continue;
		}
		const Sphere hazardSphere = hazard->GetHitSphere();
		if (IsCollision(hazardSphere, playerBody)) {
			// 攻撃物から離れる向きに吹き飛ばす
			const float direction = (playerBody.center.x >= hazardSphere.center.x) ? 1.0f : -1.0f;
			if (player_.OnDamage(direction)) {
				SpawnHitEffect(playerBody.center, kPlayerHitColor);
			}
		}
	}

	// --- ボスの体 → プレイヤー（突進中はダメージ、それ以外は重ならないよう押し出す）---
	if (boss_.IsAlive()) {
		const AABB3D bodyBox = boss_.GetBodyBox();
		if (IsCollision(bodyBox, playerBody)) {
			if (boss_.IsBodyDamaging()) {
				if (player_.OnDamage(boss_.GetFacing())) {
					SpawnHitEffect(playerBody.center, kPlayerHitColor);
				}
			} else {
				// 左右のうち近い方へ押し出す
				const float leftX = bodyBox.min.x - playerBody.radius;
				const float rightX = bodyBox.max.x + playerBody.radius;
				const bool nearLeft = std::fabs(playerBody.center.x - leftX) < std::fabs(playerBody.center.x - rightX);
				player_.SetPositionX(nearLeft ? leftX : rightX);
			}
		}
	}
}

#pragma endregion

#pragma region エフェクト

void BossScene::UpdateEffects() {
	// 前のフレームに終わったものを先に取り除く（終わったフレームは描かないので、最後に描いたフレームの
	// 描画が GPU で終わってから壊すことになる。描画中のものの定数バッファを壊さないため）
	for (auto it = effects_.begin(); it != effects_.end();) {
		if ((*it)->IsFinished()) {
			it = effects_.erase(it);
		} else {
			++it;
		}
	}

	for (std::unique_ptr<BaseEffect>& effect : effects_) {
		effect->Update();
	}
}

void BossScene::SpawnHitEffect(const Vector3& position, const Vector4& color) {
	std::unique_ptr<HitEffect> effect = std::make_unique<HitEffect>();
	effect->Initialize(position, color);
	effects_.push_back(std::move(effect));
}

void BossScene::SpawnGuardEffect(const Vector3& position) {
	std::unique_ptr<GuardEffect> effect = std::make_unique<GuardEffect>();
	effect->Initialize(position);
	effects_.push_back(std::move(effect));
}

void BossScene::SpawnBossHitEffect(Boss::HitResult result, const Vector3& attackPosition) {
	switch (result) {
	case Boss::HitResult::kBlocked:
		// 盾の位置で弾いたエフェクト
		SpawnGuardEffect(boss_.GetShieldPosition());
		break;
	case Boss::HitResult::kCoreHit:
		SpawnHitEffect(boss_.GetCoreSphere().center, kCoreHitColor);
		break;
	case Boss::HitResult::kDamaged:
		SpawnHitEffect(attackPosition, kEnemyHitColor);
		break;
	case Boss::HitResult::kNone:
	default:
		break;
	}
}

#pragma endregion

#pragma region 描画

void BossScene::OnDraw() {
	// 不透明なものを先に描く
	stage_.Draw();
	boss_.Draw();
	if (IsPlayerVisible()) {
		player_.Draw();
	}

	// 半透明のエフェクトは不透明なものの後
	if (IsPlayerVisible()) {
		player_.DrawEffect();
	}
	boss_.DrawEffect();
	for (const std::unique_ptr<BaseEffect>& effect : effects_) {
		effect->Draw();
	}
	if (deathParticles_) {
		deathParticles_->Draw();
	}

	// UI とフェードはスプライトなので一番最後（フェードが最前面）
	playerGauge_.Draw();
	bossGauge_.Draw();
	fade_.Draw();
}

#ifdef USE_IMGUI
void BossScene::OnDrawImGui() {
	ImGui::Begin("Boss Scene");
	ImGui::Text("A/D: move  W/Space: jump (hold: higher / glide)");
	ImGui::Text("LMB: melee  RMB hold: aim, wheel: curve, release: throw");
	ImGui::Text("Esc: Title  C: Clear  P: boss AI on/off  H: damage player  (development only)");
	ImGui::Text("Tuning values: Global Variables window (Save writes resources/GlobalVariables/*.csv)");
	ImGui::Separator();

	// シーンの段階とフェード
	const char* fadeName = "none";
	if (fade_.GetStatus() == Fade::Status::kFadeIn) {
		fadeName = "in";
	} else if (fade_.GetStatus() == Fade::Status::kFadeOut) {
		fadeName = "out";
	}
	const char* particlesName = "none";
	if (deathParticles_) {
		particlesName = deathParticles_->IsFinished() ? "finished" : "playing";
	}
	ImGui::Text("phase: %s  fade: %s  deathTimer: %.2f  particles: %s  effects: %d",
		GetPhaseName(), fadeName, deathTimer_, particlesName, static_cast<int>(effects_.size()));

	// プレイヤーとボスの状態を一目で見られるようにしておく（詳細は下のツリー）
	const Vector3& playerPosition = player_.GetPosition();
	const Vector3& playerVelocity = player_.GetVelocity();
	ImGui::Text("player pos: %.2f, %.2f  vel: %.3f, %.3f  grounded: %s  life: %d / %d  invincible: %s",
		playerPosition.x, playerPosition.y, playerVelocity.x, playerVelocity.y, player_.IsGrounded() ? "true" : "false",
		player_.GetLife(), player_.GetMaxLife(), player_.IsInvincible() ? "true" : "false");

	const char* attackName = "none";
	if (player_.GetAttackType() == Player::AttackType::kGround) {
		attackName = "ground";
	} else if (player_.GetAttackType() == Player::AttackType::kAir) {
		attackName = "air";
	}
	const char* boomerangPhase = "idle";
	if (player_.GetBoomerang().GetPhase() == Boomerang::Phase::kOutward) {
		boomerangPhase = "outward";
	} else if (player_.GetBoomerang().GetPhase() == Boomerang::Phase::kReturn) {
		boomerangPhase = "return";
	}
	ImGui::Text("attack: %s  aiming: %s  curve: %.2f  boomerang: %s",
		attackName, player_.IsAiming() ? "true" : "false", player_.GetCurveOffset(), boomerangPhase);
	ImGui::Text("boss hp: %d / %d  phase: %d  behavior: %s  ai: %s",
		boss_.GetHp(), boss_.GetMaxHp(), boss_.GetPhase(), boss_.GetBehaviorName(), boss_.IsAiEnabled() ? "on" : "off");
	ImGui::Text("cursor world: %.2f, %.2f", cursorWorldPosition_.x, cursorWorldPosition_.y);
	ImGui::Separator();

	stage_.DrawImGui();
	player_.DrawImGui();
	boss_.DrawImGui();
	cameraController_.DrawImGui();

	ImGui::End();
}
#endif

#pragma endregion
