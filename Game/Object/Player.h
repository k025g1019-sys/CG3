#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Object/Boomerang.h"

#include <cstdint>

class Stage;

/// <summary>
/// プレイヤー。A / D で左右移動、W または Space でジャンプ（押した長さで高さが変わる）、
/// 空中で W / Space を押し続けるとゆっくり降下する。
/// 左クリックで近接攻撃（地上は正面の円、空中は自分を中心にした大きい円）。
/// 右ボタンを押している間はカーソルを狙い、ホイールで予測線を曲げ、離すとブーメランを投げる。
/// ライフは 3。被弾すると吹き飛んでしばらく無敵になり、尽きるとその場で止まる（やられ演出はシーンが進める）。
/// 位置はモデルの translate で持ち（Z は常に 0）、速度は 1 フレームあたりの移動量。
/// </summary>
class Player {
public:

	/// <summary>
	/// 向いている方向
	/// </summary>
	enum class LRDirection {
		kRight,  // 右向き
		kLeft,   // 左向き
	};

	/// <summary>
	/// 近接攻撃の種類
	/// </summary>
	enum class AttackType {
		kNone,    // 攻撃していない
		kGround,  // 地上攻撃（正面の円）
		kAir,     // 空中攻撃（自分を中心にした大きい円）
	};

	// 近接攻撃 1 発のダメージ（ブーメランより低いが連続で出せる）
	static inline const int32_t kMeleeDamage = 1;

	/// <summary>
	/// 仮モデルを作り、ステージの床の上に置く
	/// </summary>
	/// <param name="stage">床の高さと左右の端を参照するステージ（所有しない）</param>
	void Initialize(const Stage* stage);

	/// <summary>
	/// 入力・移動・ジャンプ・攻撃・投擲の処理（毎フレーム呼ぶ）
	/// </summary>
	void Update();

	/// <summary>
	/// 本体とブーメランを描く
	/// </summary>
	void Draw() const;

	/// <summary>
	/// 半透明の攻撃エフェクトを描く（不透明なものをすべて描いた後に呼ぶ）
	/// </summary>
	void DrawEffect() const;

#ifdef USE_IMGUI
	/// <summary>
	/// 状態の表示と調整値の編集（呼び出し側の ImGui ウィンドウの中に差し込む）
	/// </summary>
	void DrawImGui();
#endif

	/// <summary>
	/// カーソルのワールド座標（Z=0 平面）を渡す。狙いと投擲に使うので Update の前に毎フレーム呼ぶ
	/// </summary>
	void SetCursorWorldPosition(const Engine::Vector3& position) { cursorWorldPosition_ = position; }

	/// <summary>
	/// ダメージを受けた（無敵中とやられた後は無視）。ライフを 1 減らし、相手から離れる向きに吹き飛んでしばらく無敵になる。
	/// ライフが尽きたら吹き飛ばずにその場で止まる
	/// </summary>
	/// <param name="knockbackDirectionX">吹き飛ぶ向き（+1 で右、-1 で左）</param>
	void OnDamage(float knockbackDirectionX);

	/// <summary>
	/// 位置の X だけを変える（ボスの体から押し出すときなど）
	/// </summary>
	void SetPositionX(float x) { model_.GetTransform().translate.x = x; }

	const Engine::Vector3& GetPosition() const { return model_.GetTransform().translate; }
	const Engine::Vector3& GetVelocity() const { return velocity_; }
	LRDirection GetDirection() const { return lrDirection_; }
	bool IsGrounded() const { return isGrounded_; }
	bool IsInvincible() const { return invincibleTimer_ > 0.0f; }
	bool IsDead() const { return life_ <= 0; }
	int32_t GetLife() const { return life_; }
	int32_t GetMaxLife() const { return kMaxLife; }

	// 当たり判定の大きさ（仮モデルの立方体に合わせてある）
	float GetWidth() const { return kWidth; }
	float GetHeight() const { return kHeight; }

	// 体の判定（敵の攻撃を受ける判定）
	Engine::Sphere GetBodySphere() const { return { GetPosition(), kBodyRadius }; }

	// 近接攻撃
	bool IsAttacking() const { return attackType_ != AttackType::kNone; }
	AttackType GetAttackType() const { return attackType_; }
	Engine::Sphere GetAttackSphere() const;  // 攻撃の判定（攻撃中だけ意味を持つ）
	bool HasAttackHit() const { return attackHasHit_; }  // 今の攻撃が既に当たったか（1 回の攻撃で 1 ヒット）
	void MarkAttackHit() { attackHasHit_ = true; }

	// 狙い・ブーメラン
	bool IsAiming() const { return isAiming_; }
	float GetCurveOffset() const { return curveOffset_; }
	const Boomerang& GetBoomerang() const { return boomerang_; }
	Boomerang& GetBoomerang() { return boomerang_; }

private:

	// --- 移動・ジャンプの調整値（1 フレームあたりの量。更新は毎秒 60 回に固定されている。ImGui で変更可）---
	static inline float kAcceleration = 0.02f;       // 左右移動の加速度
	static inline float kAttenuation = 0.15f;        // 入力が無いときの減衰率（1 フレームでこの割合だけ速度が減る）
	static inline float kBrakeAttenuation = 0.3f;    // 逆方向へ入力したときの減衰率（急ブレーキ）
	static inline float kLimitRunSpeed = 0.18f;      // 左右移動の最高速度
	static inline float kJumpSpeed = 0.25f;          // ジャンプの初速
	static inline float kJumpCutRate = 0.7f;         // 上昇中にキーを離している間、毎フレーム上昇速度に掛ける倍率（小さいほど低く跳ぶ）
	static inline float kGravity = 0.0105f;          // 重力（毎フレーム速度から引く）
	static inline float kMaxFallSpeed = 0.35f;       // 落下速度の上限
	static inline float kGlideFallSpeed = 0.06f;     // ゆっくり降下中の落下速度の上限
	static inline float kCoyoteTime = 0.1f;          // 床を離れてからジャンプを受け付ける猶予（秒）

	// --- 近接攻撃の調整値 ---
	static inline float kGroundAttackDuration = 0.5f;  // 地上攻撃の持続（秒）
	static inline float kAirAttackDuration = 0.8f;     // 空中攻撃の持続（秒）
	static inline float kGroundAttackRadius = 0.6f;    // 地上攻撃の円の半径
	static inline float kGroundAttackOffsetX = 1.0f;   // 地上攻撃の円の中心を正面へずらす量
	static inline float kAirAttackRadius = 1.2f;       // 空中攻撃の円の半径（プレイヤーより大きい）

	// --- 狙い・投擲の調整値 ---
	static inline float kCurveStepPerNotch = 1.0f;     // ホイール 1 目盛りで曲線の膨らみが変わる量
	static inline float kMaxCurveOffset = 4.0f;        // 曲線の膨らみの上限
	static inline float kThrowOffsetY = 0.2f;          // 投げ始める位置を中心から上にずらす量

	// --- 被弾の調整値 ---
	static inline float kInvincibleDuration = 1.0f;     // 被弾後の無敵時間（秒）
	static inline float kDamageKnockbackSpeed = 0.25f;  // 被弾で吹き飛ぶ横の初速
	static inline float kDamageKnockbackUpSpeed = 0.2f; // 被弾で吹き飛ぶ上の初速

	// --- 固定値 ---
	static inline const int32_t kMaxLife = 3;             // ライフの最大値
	static inline const float kWidth = 1.0f;              // 当たり判定の幅
	static inline const float kHeight = 1.0f;             // 当たり判定の高さ
	static inline const float kBodyRadius = 0.5f;         // 体の判定の半径
	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数
	static inline const float kWheelNotch = 120.0f;       // ホイール 1 目盛りぶんの入力値

	Engine::Object3D model_;                              // 見た目（仮モデル：立方体）。translate が位置
	Engine::Object3D attackMark_;                         // 攻撃判定の見た目（半透明の球）
	const Stage* stage_ = nullptr;                        // 床の高さ・左右の端（借りるだけ）

	Engine::Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };     // 速度（1 フレームあたりの移動量）
	LRDirection lrDirection_ = LRDirection::kRight;       // 向いている方向
	bool isGrounded_ = false;                             // 床に着いているか
	bool isGliding_ = false;                              // ゆっくり降下中か
	float coyoteTimer_ = 0.0f;                            // 床を離れてからの経過時間（秒）

	AttackType attackType_ = AttackType::kNone;           // 現在の近接攻撃
	float attackTimer_ = 0.0f;                            // 近接攻撃の残り時間（秒）
	bool attackHasHit_ = false;                           // 今の近接攻撃が既に当たったか

	Engine::Vector3 cursorWorldPosition_ = { 0.0f, 0.0f, 0.0f };  // カーソルのワールド座標（Z=0 平面。シーンから毎フレーム受け取る）
	bool isAiming_ = false;                               // 右ボタンを押して狙っている最中か
	float curveOffset_ = 0.0f;                            // 予測線の曲がり具合（線の中点からの膨らみ）
	Boomerang boomerang_;                                 // ブーメラン（同時に 1 つ）

	float invincibleTimer_ = 0.0f;                        // 被弾後の無敵の残り時間（秒）
	int32_t life_ = kMaxLife;                             // 残りライフ（0 でやられ）

	// 左右の入力で加速・減速する
	void UpdateMove();

	// ジャンプの開始・高さの調整・ゆっくり降下
	void UpdateJump();

	// 重力を掛けて位置を進める
	void UpdatePhysics();

	// 床への着地と、左右の端での停止
	void UpdateCollision();

	// 近接攻撃の開始と持続の管理
	void UpdateAttack();

	// 右ボタンでの狙い、ホイールでの曲線化、離して投げる
	void UpdateAim();

	// 向きをモデルに反映する（狙っている間はカーソルの方を向く）
	void UpdateDirection();

	// 無敵時間の経過と点滅
	void UpdateInvincible();

	// 攻撃エフェクトの位置と大きさを判定に合わせる
	void UpdateAttackMark();

	// 投げ始める位置
	Engine::Vector3 GetThrowOrigin() const;

	// 近接攻撃の判定の中心
	Engine::Vector3 CalcAttackCenter() const;

	// ジャンプ入力（W または Space）
	bool IsJumpTrigger() const;
	bool IsJumpPress() const;
};
