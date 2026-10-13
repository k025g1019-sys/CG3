#pragma once

#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Scene/BaseScene.h"
#include "Game/Effect/BaseEffect.h"
#include "Game/Effect/DeathParticles.h"
#include "Game/Object/Boss.h"
#include "Game/Object/Player.h"
#include "Game/Stage/CameraController.h"
#include "Game/Stage/Stage.h"
#include "Game/UI/Fade.h"
#include "Game/UI/Gauge.h"

#include <memory>
#include <vector>

/// <summary>
/// ボス戦のシーン。ステージ・プレイヤー・ボス・カメラ・UI を持ち、オブジェクト同士の当たり判定をまとめて行う。
/// カーソルの位置をゲームの平面（Z=0）上のワールド座標に直してプレイヤーへ渡す。
/// プレイヤーのライフが尽きると やられ演出 → フェードアウト → タイトルへ、ボスを倒すと フェードアウト → クリア画面へ。
/// 開発中は Esc でタイトルへ、C でクリアへ、P でボスの AI の ON / OFF、H でプレイヤーにダメージ。
/// </summary>
class BossScene : public Engine::BaseScene {
public:

	// 調整項目を登録（このシーンが持つオブジェクトの分もまとめて。起動時にファイルを読み込んだ後に一度呼ぶ）
	static void RegisterGlobalVariables();
	// 調整項目を適用（このシーンが持つオブジェクトの分もまとめて。毎フレーム呼ぶ）
	static void ApplyGlobalVariables();

protected:

	void OnInitialize() override;

	void OnUpdate() override;

	void OnDraw() override;

#ifdef USE_IMGUI
	void OnDrawImGui() override;
#endif

private:

	/// <summary>
	/// シーンの段階
	/// </summary>
	enum class Phase {
		kPlay,   // 戦闘中（開始時のフェードインも含む）
		kDeath,  // プレイヤーのやられ演出（止まる → パーティクル → フェードアウト → タイトル）
		kClear,  // ボスを倒した（フェードアウト → クリア画面）
	};

	// --- 調整値 ---
	static inline const float kFadeDuration = 1.0f;       // フェードにかける時間（秒）
	static inline float kDeathFreezeDuration = 0.5f;      // やられた瞬間に止まって見せる時間（秒）
	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数

	// --- UI の配置（1280x720 基準の px。左上の角）---
	static inline const Engine::Vector2 kPlayerGaugePosition = { 40.0f, 40.0f };
	static inline const Engine::Vector2 kPlayerGaugeSize = { 260.0f, 22.0f };
	static inline const Engine::Vector2 kBossGaugePosition = { 340.0f, 664.0f };
	static inline const Engine::Vector2 kBossGaugeSize = { 600.0f, 20.0f };

	Stage stage_;                         // 床と左右の端
	Player player_;                       // 操作キャラクター
	Boss boss_;                           // ボス
	CameraController cameraController_;   // シーンのカメラをプレイヤーに追従させる

	Gauge playerGauge_;                   // プレイヤーのライフ（左上）
	Gauge bossGauge_;                     // ボスの HP（下）
	Fade fade_;                           // シーンの出入りのフェード
	std::unique_ptr<DeathParticles> deathParticles_;  // やられ演出（やられたときに作る）
	std::vector<std::unique_ptr<BaseEffect>> effects_;  // ヒットや弾きのエフェクト（終わったものは取り除く）

	Phase phase_ = Phase::kPlay;          // 現在の段階
	float deathTimer_ = 0.0f;             // やられてからの時間（秒）

	Engine::Vector3 cursorWorldPosition_ = { 0.0f, 0.0f, 0.0f };  // カーソルのワールド座標（Z=0 平面）

	// 段階ごとの更新
	void UpdatePlay();
	void UpdateDeath();
	void UpdateClear();

	// 段階の切り替え
	void StartDeath();
	void StartClear();

	// カーソルの位置をワールド座標に直してプレイヤーへ渡し、ステージ・プレイヤー・ボスを更新する
	void UpdateObjects();

	// カーソルのスクリーン座標をゲームの平面上のワールド座標に変換する
	Engine::Vector3 CalcCursorWorldPosition() const;

	// すべての当たり判定（プレイヤーの攻撃 → ボス・攻撃物、ボスの攻撃 → プレイヤー、体の押し出し）
	void CheckAllCollisions();

	// エフェクトの更新と、終わったものの削除
	void UpdateEffects();

	// エフェクトの発生
	void SpawnHitEffect(const Engine::Vector3& position, const Engine::Vector4& color);
	void SpawnGuardEffect(const Engine::Vector3& position);
	void SpawnBossHitEffect(Boss::HitResult result, const Engine::Vector3& attackPosition);  // ボスへの攻撃の結果に合わせて出す

	// プレイヤーを描くか（やられ演出で止まって見せた後は消す）
	bool IsPlayerVisible() const;

	const char* GetPhaseName() const;
};
