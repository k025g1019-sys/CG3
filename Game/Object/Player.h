#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Object/Boomerang.h"
#include "Game/Util/HitStop.h"

#include <cstdint>

class Stage;

/// <summary>
/// プレイヤー。A / D で左右移動、W または Space でジャンプ（押した長さで高さが変わる）、
/// 空中で W / Space を押し続けるとゆっくり降下する。
/// 左クリックで近接攻撃（地上は正面の円、空中は自分を中心にした大きい円）。
/// 右ボタンを押している間はカーソルを狙い、ホイールで予測線を曲げ、離すとブーメランを投げる。
/// ブーメランが飛んでいる間に右ボタンを押すと、押した瞬間からカーソルを追いかけさせる（追従）。すぐ離すと大技になる。
/// 狙っている間も近接攻撃は出せるが、近接攻撃の最中はブーメランを投げられない。
/// ブーメランをキャッチした直後は少しの間、近接攻撃も狙いも出せない（大技を出した投げでは、狙いだけ少し長く待つ）。
/// ライフは調整値（既定 6）。被弾すると一瞬止まって揺れ（ヒットストップ）、続けて攻撃から離れる向きへ少しずれ（ノックバック）、しばらく無敵になる。
/// ライフが尽きるとその場で止まる（やられ演出はシーンが進める）。
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

	// 近接攻撃 1 発のダメージ（ブーメランより低いが連続で出せる。調整値）
	static inline int32_t kMeleeDamage = 4;

	/// <summary>
	/// 仮モデルを作り、ステージの床の上に置く
	/// </summary>
	/// <param name="stage">床の高さと左右の端を参照するステージ（所有しない）</param>
	void Initialize(const Stage* stage);

	/// <summary>
	/// 最初の状態に戻す（位置・ライフ・攻撃など。リトライで使う。モデルは作り直さない）
	/// </summary>
	void Reset();

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
	/// 状態の表示（呼び出し側の ImGui ウィンドウの中に差し込む。調整値の編集は Global Variables ウィンドウで行う）
	/// </summary>
	void DrawImGui();
#endif

	// 調整項目を登録（既定値を GlobalVariables へ追加する。ファイル読み込み後に一度呼ぶ）
	static void RegisterGlobalVariables();
	// 調整項目を適用（GlobalVariables の現在値を調整項目へ反映する。毎フレーム呼ぶ）
	static void ApplyGlobalVariables();

#ifdef USE_IMGUI
	/// <summary>
	/// ホイールを回す向きと曲線が曲がる向きの対応を切り替えるチェックボックス（呼び出し側の ImGui ウィンドウの中に差し込む）。
	/// 遊びながらすぐ切り替えて比べるためのもので、値は GlobalVariables の Player / WheelAwayCurvesUp に入る（Save で CSV に保存される）
	/// </summary>
	static void DrawWheelDirectionImGui();
#endif

	/// <summary>
	/// カーソルのワールド座標（Z=0 平面）を渡す。狙いと投擲に使うので Update の前に毎フレーム呼ぶ
	/// </summary>
	void SetCursorWorldPosition(const Engine::Vector3& position) { cursorWorldPosition_ = position; }

	/// <summary>
	/// ダメージを受けた（無敵中とやられた後は無視）。ライフを 1 減らし、一瞬止まって揺れ（ヒットストップ）、
	/// 続けて攻撃から離れる向きへ少しずれる（ノックバック。その間は操作できない）。そのあとしばらく無敵になる。
	/// 無敵時間は止まっている間は減らず、ノックバックが始まってから数え始める。
	/// ライフが尽きたらその場で止まる（ノックバックしない）。開発用の無敵モードではライフを減らさない
	/// </summary>
	/// <param name="knockbackDirectionX">ノックバックでずれる向き（+1 で右、-1 で左。攻撃から離れる向きを渡す）</param>
	/// <returns>ダメージの処理をしたら true（無敵中などで無視したら false）</returns>
	bool OnDamage(float knockbackDirectionX);

	/// <summary>
	/// 操作を受け付けるかを切り替える。false にすると新しい行動（移動・ジャンプ・近接・狙い）を始めなくなる。
	/// 進行中の行動は最後まで行う（移動は減速して止まり、空中なら着地し、近接は終わりまで、投げたブーメランは戻ってくる）。
	/// 勝敗がついた後に使う
	/// </summary>
	void SetControllable(bool isControllable);
	bool IsControllable() const { return isControllable_; }

	/// <summary>
	/// 見た目だけを更新する（止めて見せる間も毎フレーム呼ぶ。描くものは毎フレーム Update が必要なため）
	/// </summary>
	void UpdateModel();

	// 開発用の無敵モード（ダメージを受けないが、ヒットストップと無敵の演出はする）
	void SetInvincibleMode(bool enabled) { isInvincibleMode_ = enabled; }
	bool IsInvincibleMode() const { return isInvincibleMode_; }

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
	static inline float kLimitRunSpeed = 0.16f;      // 左右移動の最高速度
	static inline float kJumpSpeed = 0.424f;         // ジャンプの初速（2026-10-27 に高さ 2 → 1 で立つ位置が 0.5 下がったので、最高点が前と同じになるよう 0.4 から上げた）
	static inline float kJumpCutRate = 0.87f;        // 上昇中にキーを離している間、毎フレーム上昇速度に掛ける倍率（小さいほど低く跳ぶ。同じ理由で 0.8 から上げ、小ジャンプの最高点も前と同じにした）
	static inline float kGravity = 0.02f;            // 重力（毎フレーム速度から引く）
	static inline float kMaxFallSpeed = 0.32f;       // 落下速度の上限
	static inline float kGlideFallSpeed = 0.04f;     // ゆっくり降下中の落下速度の上限
	static inline float kCoyoteTime = 0.1f;          // 床を離れてからジャンプを受け付ける猶予（秒）
	static inline float kApexSpeedThreshold = 0.07f; // 上下の速度がこれより小さい間は頂点付近とみなす
	static inline float kApexGravityRate = 0.5f;     // 頂点付近で重力に掛ける倍率（小さいほど頂点に長くいる）
	static inline float kJumpBufferTime = 0.2f;      // ジャンプの先行入力を覚えておく時間（秒）

	// --- 近接攻撃の調整値 ---
	static inline float kGroundAttackDuration = 0.5f;  // 地上攻撃の持続（秒）
	static inline float kAirAttackDuration = 0.7f;     // 空中攻撃の持続（秒）
	static inline float kGroundAttackRadius = 1.5f;    // 地上攻撃の円の半径
	static inline float kGroundAttackOffsetX = 1.8f;   // 地上攻撃の円の中心を正面へずらす量
	static inline float kAirAttackRadius = 1.5f;       // 空中攻撃の円の半径（プレイヤーより大きい）
	static inline float kAttackJumpLockTime = 0.15f;   // 地上攻撃を出してからジャンプできない時間（秒）

	// --- 狙い・投擲の調整値 ---
	static inline float kCurveStepPerNotch = 1.0f;     // ホイール 1 目盛りで曲線の膨らみが変わる量
	static inline int32_t kWheelAwayCurvesUp = 0;      // ホイールを奥へ回したときに曲がる向き（1 で上向き、0 で下向き。手前へ回すとその逆）
	static inline float kMaxCurveOffset = 4.0f;        // 曲線の膨らみの上限
	static inline float kThrowOffsetY = 0.2f;          // 投げ始める位置を中心から上にずらす量
	static inline float kCatchCooldown = 0.25f;        // ブーメランをキャッチしてから、次の攻撃（近接・狙い）ができるまでの時間（秒）
	static inline float kBurstCatchCooldown = 0.4f;    // 大技を出したブーメランをキャッチしてから、次に狙えるまでの時間（秒。近接は CatchCooldown のまま）
	static inline float kBurstClickTime = 0.15f;       // 飛んでいるブーメランへの右ボタンを、押してからこの時間以内に離したら大技（秒）

	// --- 被弾の調整値 ---
	static inline int32_t kMaxLife = 6;                 // ライフの最大値
	static inline float kInvincibleDuration = 1.0f;     // 被弾後の無敵時間（秒。ヒットストップが終わってから数える）
	static inline float kDamageHitStopDuration = 0.15f; // 被弾したときに止まる時間（秒）
	static inline float kDamageShakeAmplitude = 0.12f;  // 被弾したときの揺れ幅
	static inline float kDamageKnockbackSpeed = 0.2f;   // ノックバックの初速（1 フレームあたり。終わりにかけて 0 へ落ちる）
	static inline float kDamageKnockbackDuration = 0.2f;  // ノックバックの時間（秒。その間は操作できない）

	// --- 固定値 ---
	static inline const float kWidth = 1.0f;              // 当たり判定の幅
	static inline const float kHeight = 1.0f;             // 当たり判定の高さ（見た目の立方体と同じ。床に立つ位置は床 + 高さの半分）
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
	float jumpBufferTimer_ = 0.0f;                        // ジャンプの先行入力の残り時間（秒）

	AttackType attackType_ = AttackType::kNone;           // 現在の近接攻撃
	float attackTimer_ = 0.0f;                            // 近接攻撃の残り時間（秒）
	bool attackHasHit_ = false;                           // 今の近接攻撃が既に当たったか
	float attackJumpLockTimer_ = 0.0f;                    // 地上攻撃を出した後にジャンプできない残り時間（秒）

	Engine::Vector3 cursorWorldPosition_ = { 0.0f, 0.0f, 0.0f };  // カーソルのワールド座標（Z=0 平面。シーンから毎フレーム受け取る）
	bool isAiming_ = false;                               // 右ボタンを押して狙っている最中か
	float curveOffset_ = 0.0f;                            // 予測線の曲がり具合（線の中点からの膨らみ）
	Boomerang boomerang_;                                 // ブーメラン（同時に 1 つ）
	float meleeCooldownTimer_ = 0.0f;                     // キャッチ後、近接攻撃ができるまでの残り時間（秒）
	float aimCooldownTimer_ = 0.0f;                       // キャッチ後、狙いを始められるまでの残り時間（秒。大技の後は少し長い）
	float followPressTimer_ = 0.0f;                       // 追従を始めてから（右ボタンを押してから）の時間（秒。短ければ大技にする）

	float invincibleTimer_ = 0.0f;                        // 被弾後の無敵の残り時間（秒）
	int32_t life_ = kMaxLife;                             // 残りライフ（0 でやられ）
	bool isInvincibleMode_ = false;                       // 開発用：ダメージを受けない（ヒットストップと無敵の演出はする）
	HitStop hitStop_;                                     // ヒットストップ（被弾したとき）
	bool isKnockbackPending_ = false;                     // ヒットストップが終わったらノックバックを始めるか
	float knockbackDirection_ = 1.0f;                     // ノックバックでずれる向き（+1 で右、-1 で左）
	float knockbackTimer_ = 0.0f;                         // ノックバックの残り時間（秒）
	bool isControllable_ = true;                          // 操作を受け付けるか（勝敗がついた後は false）

	// 左右の入力で加速・減速する
	void UpdateMove();

	// ノックバック：攻撃から離れる向きへ滑る（始まりの判定と、時間の経過も行う）
	void UpdateKnockback();

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

	// 飛んでいるブーメランの操作（押している間は追従、押してすぐ離したら大技、長く押してから離したら戻す）
	void UpdateBoomerangControl();

	// 向きをモデルに反映する（狙っている間はカーソルの方を向く）
	void UpdateDirection();

	// 無敵時間の経過と点滅
	void UpdateInvincible();

	// ブーメランの更新と、キャッチした瞬間のクールタイム開始
	void UpdateBoomerang();

	// 本体のモデルを更新する（ヒットストップ中は揺れのぶん見た目だけずらす）
	void UpdateBodyModel();

	// 攻撃エフェクトの位置と大きさを判定に合わせる
	void UpdateAttackMark();

	// 狙いの予測線と、ブーメランが折り返す位置の印を描く（狙っている間は毎フレーム呼ぶ）
	void DrawAimLine() const;

	// 投げ始める位置
	Engine::Vector3 GetThrowOrigin() const;

	// 近接攻撃の判定の中心
	Engine::Vector3 CalcAttackCenter() const;

	// 新しく近接攻撃を始められるか（操作できて、ブーメランが手元にあり、キャッチ直後でない）
	bool CanStartMelee() const;

	// 新しく狙いを始められるか（操作できて、ブーメランが手元にあり、キャッチ直後でない。大技の後は近接より少し長く待つ）
	bool CanStartAim() const;

	// ノックバックの最中か
	bool IsKnockback() const { return knockbackTimer_ > 0.0f; }

	// 操作できるか（勝敗がついた後と、ノックバックの最中はできない）
	bool CanControl() const { return isControllable_ && !IsKnockback(); }

	// ジャンプ入力（W または Space）
	bool IsJumpTrigger() const;
	bool IsJumpPress() const;
};
