#include "Game/Scene/BossScene.h"

#include "Engine/Input/Input.h"
#include "Engine/Math/Collision.h"
#include "Game/Scene/ClearScene.h"
#include "Game/Scene/TitleScene.h"
#include "Game/Util/ScreenToWorld.h"

#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

const Vector4 kPlayerGaugeColor = { 0.3f, 0.9f, 0.4f, 1.0f };  // プレイヤーのライフの色
const Vector4 kBossGaugeColor = { 0.9f, 0.25f, 0.25f, 1.0f };  // ボスの HP の色

} // namespace

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
}

#pragma endregion

#pragma region 更新

void BossScene::OnUpdate() {
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
		player_.OnDamage(1.0f);
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

	// カメラはプレイヤーが動いた後に追従させる
	cameraController_.Update(GetAspectRatio());

	// UI はライフと HP の割合を映す
	playerGauge_.Update(static_cast<float>(player_.GetLife()) / static_cast<float>(player_.GetMaxLife()));
	bossGauge_.Update(static_cast<float>(boss_.GetHp()) / static_cast<float>(boss_.GetMaxHp()));

	// フェード。フェードインが終わったら止める（戦闘中は最前面のスプライトを描かない）
	fade_.Update();
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
		const Boss::HitResult result = boss_.ApplyAttack(player_.GetAttackSphere(), Player::kMeleeDamage);
		if (result != Boss::HitResult::kNone) {
			player_.MarkAttackHit();
		}
	}

	// --- ブーメラン → ボス（往路・復路で 1 回ずつ。盾に当たれば弾かれて戻る）---
	if (boomerang.CanHit()) {
		const Boss::HitResult result = boss_.ApplyAttack(boomerang.GetHitSphere(), Boomerang::kDamage);
		if (result == Boss::HitResult::kBlocked) {
			boomerang.Repel();
		}
		if (result != Boss::HitResult::kNone) {
			boomerang.MarkHit();
		}
	}

	// --- プレイヤーの攻撃 → 壊せる攻撃物（岩・ビット）---
	for (const std::unique_ptr<Hazard>& hazard : boss_.GetHazards()) {
		if (!hazard->IsAlive() || !hazard->CanBeDestroyed()) {
			continue;
		}
		const Sphere hazardSphere = hazard->GetHitSphere();
		if (player_.IsAttacking() && IsCollision(player_.GetAttackSphere(), hazardSphere)) {
			hazard->OnHitByPlayer();
		} else if (boomerang.IsFlying() && IsCollision(boomerang.GetHitSphere(), hazardSphere)) {
			hazard->OnHitByPlayer();
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
			player_.OnDamage(direction);
		}
	}

	// --- ボスの体 → プレイヤー（突進中はダメージ、それ以外は重ならないよう押し出す）---
	if (boss_.IsAlive()) {
		const AABB3D bodyBox = boss_.GetBodyBox();
		if (IsCollision(bodyBox, playerBody)) {
			if (boss_.IsBodyDamaging()) {
				player_.OnDamage(boss_.GetFacing());
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
	ImGui::Text("phase: %s  fade: %s  deathTimer: %.2f  particles: %s", GetPhaseName(), fadeName, deathTimer_, particlesName);

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

	if (ImGui::TreeNode("Scene")) {
		ImGui::DragFloat("deathFreezeDuration", &kDeathFreezeDuration, 0.05f, 0.0f, 3.0f);
		ImGui::TreePop();
	}
	stage_.DrawImGui();
	player_.DrawImGui();
	boss_.DrawImGui();
	cameraController_.DrawImGui();

	ImGui::End();
}
#endif

#pragma endregion
