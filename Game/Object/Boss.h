#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Object/Hazard.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <vector>

class Player;
class Stage;

/// <summary>
/// ボス「盾持ちの守護像」。正面に盾を構え、背中に弱点のコアを持つ。
/// 待機 → 攻撃（突進 / 叩きつけ / 落石 の順）→ 待機 … を繰り返し、突進のあとは疲れて盾が下がる。
/// 正面からの攻撃は盾に弾かれ、背後からの攻撃とコアへの攻撃（ダメージ 2 倍）が通る。コアに当たると怯む。
/// HP が 2/3 以下で浮遊ビットを呼び、1/3 以下で突進が 2 連続になる。
/// </summary>
class Boss {
public:

	/// <summary>
	/// 振る舞い（状態）
	/// </summary>
	enum class Behavior {
		kIdle,      // 待機（プレイヤーの方へ向き直り、次の攻撃を待つ）
		kCharge,    // 突進（予備動作 → 走る）
		kTired,     // 突進後の疲れ（盾が下がる）
		kSlam,      // 叩きつけ（衝撃波を出す）
		kRockFall,  // 落石（上空から岩を落とす）
		kStagger,   // 怯み（コアに攻撃を受けた）
		kDead,      // やられ
	};

	/// <summary>
	/// プレイヤーの攻撃を当てた結果
	/// </summary>
	enum class HitResult {
		kNone,     // 当たっていない
		kBlocked,  // 盾に弾かれた
		kDamaged,  // 体に当たった
		kCoreHit,  // 弱点のコアに当たった
	};

	/// <summary>
	/// 仮モデルを作り、ステージの右寄りに置く
	/// </summary>
	/// <param name="stage">床の高さと左右の端を参照するステージ（所有しない）</param>
	/// <param name="player">狙う相手（所有しない）</param>
	void Initialize(const Stage* stage, const Player* player);

	/// <summary>
	/// 状態機械と攻撃物の更新（毎フレーム呼ぶ）
	/// </summary>
	void Update();

	/// <summary>
	/// 体・コア・盾・攻撃物を描く
	/// </summary>
	void Draw() const;

#ifdef USE_IMGUI
	/// <summary>
	/// 状態の表示と調整値の編集（呼び出し側の ImGui ウィンドウの中に差し込む）
	/// </summary>
	void DrawImGui();
#endif

	/// <summary>
	/// プレイヤーの攻撃（近接・ブーメラン）を当てる。弱点 → 体 の順に調べて結果を返す。
	/// 盾が有効で正面からの攻撃なら弾く
	/// </summary>
	/// <param name="attackSphere">攻撃の判定</param>
	/// <param name="damage">基本ダメージ（コアに当たると 2 倍）</param>
	HitResult ApplyAttack(const Engine::Sphere& attackSphere, int32_t damage);

	// 体の判定（箱）
	Engine::AABB3D GetBodyBox() const;

	// 弱点のコアの判定（背中側）
	Engine::Sphere GetCoreSphere() const;

	// 盾が有効か（疲れ・怯み・やられの間は無効）
	bool IsGuarding() const;

	// 体に触れるとダメージを受ける状態か（突進で走っている間）
	bool IsBodyDamaging() const;

	bool IsAlive() const { return hp_ > 0; }
	bool IsDeathFinished() const { return isDeathFinished_; }
	int32_t GetHp() const { return hp_; }
	int32_t GetMaxHp() const { return kMaxHp; }
	int32_t GetPhase() const { return phase_; }
	Behavior GetBehavior() const { return behavior_; }
	const char* GetBehaviorName() const;
	float GetFacing() const { return facing_; }
	const Engine::Vector3& GetPosition() const { return position_; }
	const std::vector<std::unique_ptr<Hazard>>& GetHazards() const { return hazards_; }

	// 開発用：待機から攻撃に移るかどうか
	void SetAiEnabled(bool enabled) { isAiEnabled_ = enabled; }
	bool IsAiEnabled() const { return isAiEnabled_; }

private:

	// 突進の段階
	enum class ChargeStep {
		kWindup,  // 予備動作（後ろへ傾く）
		kDash,    // 走る
	};

	// 叩きつけの段階
	enum class SlamStep {
		kWindup,   // 振りかぶる
		kRecover,  // 余韻
	};

	// 落石の段階
	enum class RockFallStep {
		kRoar,     // 吠える（予備動作）
		kRecover,  // 余韻
	};

	// --- 固定値 ---
	static inline const int32_t kMaxHp = 24;               // 最大 HP
	static inline const int32_t kWeakPointMultiplier = 2;  // コアに当たったときのダメージ倍率
	static inline const int32_t kAttackPatternCount = 3;   // 攻撃の種類（突進 / 叩きつけ / 落石）
	static inline const int32_t kRockCount = 4;            // 落石 1 回で落とす岩の数
	static inline const int32_t kBitCount = 3;             // 第 2 段階で呼ぶビットの数
	static inline const float kFrameTime = 1.0f / 60.0f;   // 1 フレームの秒数

	// --- 体の調整値 ---
	static inline float kBodyHalfWidth = 1.0f;     // 体の半分の幅
	static inline float kBodyHalfHeight = 1.2f;    // 体の半分の高さ（プレイヤーのジャンプで越えられる高さにしておく）
	static inline float kShieldOffsetX = 1.15f;    // 盾の位置（中心から正面へ）
	static inline float kCoreOffsetX = 1.2f;       // コアの位置（中心から背中へ）
	static inline float kCoreOffsetY = 0.4f;       // コアの高さ（中心から上へ）
	static inline float kCoreRadius = 0.45f;       // コアの半径

	// --- 行動の調整値（秒、1 フレームあたりの量）---
	static inline float kFirstIdleDuration = 1.5f;  // 戦闘開始から最初の攻撃までの待ち
	static inline float kIdleDuration = 1.2f;       // 攻撃と攻撃の間の待ち
	static inline float kTurnDelay = 0.5f;          // 待機中、背後に回られてから向き直るまでの遅れ
	static inline float kChargeWindup = 0.6f;       // 突進の予備動作
	static inline float kChargeSpeed = 0.3f;        // 突進の速さ
	static inline float kChargeDistance = 14.0f;    // 突進で走る距離の上限（壁に着いても止まる）
	static inline float kChargeMinDistance = 3.0f;  // これより近いと突進ではなく叩きつけにする
	static inline float kTiredDuration = 1.5f;      // 突進後の疲れ（盾が下がる時間）
	static inline float kSlamWindup = 0.7f;         // 叩きつけの振りかぶり
	static inline float kSlamRecover = 0.8f;        // 叩きつけの余韻
	static inline float kSlamRange = 5.0f;          // これより遠いと叩きつけではなく突進にする
	static inline float kRockRoar = 0.5f;           // 落石の予備動作
	static inline float kRockRecover = 1.0f;        // 落石の余韻
	static inline float kRockSpawnHeight = 11.0f;   // 岩の出現する高さ
	static inline float kRockHeightStep = 2.0f;     // 岩ごとの高さの差（落ちる時間をずらす）
	static inline float kRockSpreadX = 3.0f;        // 岩を散らす横幅（プレイヤーの左右）
	static inline float kStaggerDuration = 0.5f;    // 怯みの時間
	static inline float kDeathDuration = 1.5f;      // やられ演出の時間
	static inline float kDeathSinkSpeed = 0.02f;    // やられ演出で沈む速さ
	static inline float kFlashDuration = 0.15f;     // ダメージを受けたときの白い点滅
	static inline float kBitAnchorOffsetY = 1.0f;   // ビットが回る中心の高さ（中心から上へ）

	Engine::Object3D body_;    // 体（仮モデル：箱）
	Engine::Object3D shield_;  // 盾（仮モデル：薄い板。盾が有効なときだけ描く）
	Engine::Object3D core_;    // 弱点のコア（仮モデル：球）

	const Stage* stage_ = nullptr;    // 床の高さ・左右の端（借りるだけ）
	const Player* player_ = nullptr;  // 狙う相手（借りるだけ）

	Engine::Vector3 position_ = { 0.0f, 0.0f, 0.0f };  // 体の中心
	float facing_ = -1.0f;                             // 向き（+1 で右、-1 で左）

	Behavior behavior_ = Behavior::kIdle;              // 現在の振る舞い
	std::optional<Behavior> behaviorRequest_;          // 次のフレームで切り替える振る舞い
	float timer_ = 0.0f;                               // 振る舞いの経過時間（秒）
	float idleDuration_ = 0.0f;                        // 今回の待機時間
	float turnTimer_ = 0.0f;                           // 背後に回られてからの時間
	ChargeStep chargeStep_ = ChargeStep::kWindup;      // 突進の段階
	int32_t chargeRemaining_ = 0;                      // 残りの突進回数（第 3 段階は 2 連続）
	float chargeStartX_ = 0.0f;                        // 走り始めた X
	SlamStep slamStep_ = SlamStep::kWindup;            // 叩きつけの段階
	RockFallStep rockFallStep_ = RockFallStep::kRoar;  // 落石の段階
	int32_t attackCycle_ = 0;                          // 攻撃の順番（突進 → 叩きつけ → 落石 → …）

	int32_t hp_ = kMaxHp;                              // 残り HP
	int32_t phase_ = 1;                                // 段階（1〜3）
	bool isDeathFinished_ = false;                     // やられ演出が終わったか
	float flashTimer_ = 0.0f;                          // ダメージの白い点滅の残り時間
	bool isAiEnabled_ = true;                          // 開発用：攻撃に移るか

	std::vector<std::unique_ptr<Hazard>> hazards_;     // 出した攻撃物（衝撃波・岩・ビット）
	std::mt19937 randomEngine_;                        // 岩の位置を散らす乱数

	// 振る舞いを切り替えたときの初期化
	void StartBehavior(Behavior next);

	// 振る舞いごとの更新
	void UpdateIdle();
	void UpdateCharge();
	void UpdateTired();
	void UpdateSlam();
	void UpdateRockFall();
	void UpdateStagger();
	void UpdateDead();

	// 次の攻撃を選ぶ（順番に出し、距離に合わないものは差し替える）
	void ChooseNextAttack();

	// すぐにプレイヤーの方を向く
	void FacePlayer();

	// 待機中：背後に回られたら少し遅れて向き直る
	void UpdateTurnToPlayer();

	// HP に応じて段階を進める
	void UpdatePhase();

	// 攻撃物の更新と、消えたものの削除
	void UpdateHazards();

	// 見た目の位置・色を状態に合わせる
	void UpdateModels();

	void TakeDamage(int32_t damage);

	// 点が正面側（盾の側）にあるか
	bool IsInFront(const Engine::Vector3& point) const;

	// プレイヤーがいる側（+1 で右、-1 で左）
	float CalcPlayerSide() const;

	// 体の中心が動ける範囲
	float GetMinX() const;
	float GetMaxX() const;

	void SpawnShockwave();
	void SpawnRocks();
	void SpawnBits();
	void KillAllHazards();
};
