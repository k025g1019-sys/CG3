#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Object/Hazard.h"
#include "Game/Util/HitStop.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <vector>

class Player;
class Stage;

/// <summary>
/// ボス「盾持ちの守護像」。正面に盾を構え、背中に弱点のコアを持つ。
/// 待機 → 攻撃（突進 / 叩きつけ / 落石 / 押しつぶし の順）→ 待機 … を繰り返し、突進のあとは疲れて盾が下がる。
/// 正面からの攻撃は盾に弾かれ、背後からの攻撃とコアへの攻撃（ダメージ 2 倍）が通る。
/// コアに当たると怯むが、攻撃の途中は怯まない（怯むのは 待機中・疲れ中・押しつぶしで浮いている間 だけ）。
/// 押しつぶしはプレイヤーの頭上へ跳んで浮いてから落ちてくる。浮いている間も盾は正面にあるので、
/// 下向きに曲げたブーメランで下から回り込むとコアを狙える。浮いている間に、追従を使っていない投げ（通常の投げか大技）が体かコアに当たると打ち落とされ、
/// 体を傾けながらゆっくり床へ落ち、着地してから怯む（追従を使った投げでは打ち落とせない）。
/// ダメージを受けると一瞬止まって揺れる（ヒットストップ。行動は止まるだけで打ち切られない）。
/// 近接攻撃を短い間隔で続けて受けるとヒットストップに耐性がつき、揺れるだけで止まらなくなる（しばらく近接を受けなければ戻る）。
/// チュートリアルの間は眠って宙に浮き、盾を体の下に横へ広げて構える。体の下から盾に当たった攻撃は弾き、曲げたブーメランで当てると起きて床へ落ち、戦闘が始まる（まっすぐ投げたものが当たっても身じろぎするだけ）。
/// HP が半分以下（第 2 段階）で、叩きつけのあとに浮遊ビットを呼び、押しつぶしの着地のあとに岩を降らせる。1/3 以下（第 3 段階）で突進が 2 連続になる。
/// </summary>
class Boss {
public:

	/// <summary>
	/// 振る舞い（状態）
	/// </summary>
	enum class Behavior {
		kSleep,     // 眠っている（チュートリアル。宙に浮いたまま動かず、下からの攻撃は盾で弾く）
		kIdle,      // 待機（プレイヤーの方へ向き直り、次の攻撃を待つ）
		kCharge,    // 突進（予備動作 → 走る）
		kTired,     // 突進後の疲れ（盾が下がる）
		kSlam,      // 叩きつけ（衝撃波を出す）
		kRockFall,  // 落石（上空から岩を落とす）
		kCrush,     // 押しつぶし（プレイヤーの頭上へ跳び、浮いてから落ちる）
		kStagger,   // 怯み（コアに攻撃を受けた）
		kDead,      // やられ
	};

	/// <summary>
	/// このフレームに床へ着いたか（シーンがカメラを揺らすのに使う）
	/// </summary>
	enum class Landing {
		kNone,      // 着いていない
		kCrush,     // 押しつぶしで落ちてきて着いた
		kShotDown,  // 押しつぶしの途中で打ち落とされて着いた
		kWakeUp,    // 起こされて落ちてきて着いた
	};

	/// <summary>
	/// プレイヤーの攻撃を当てた結果
	/// </summary>
	enum class HitResult {
		kNone,     // 当たっていない
		kBlocked,  // 盾に弾かれた
		kDamaged,  // 体に当たった
		kCoreHit,  // 弱点のコアに当たった
		kWokenUp,  // 眠っているところに当たって起こした（ダメージはなし）
		kStirred,  // 眠っているところに当たったが、起こせない攻撃だったので身じろぎしただけ（ダメージはなし）
	};

	/// <summary>
	/// プレイヤーの攻撃の種類（ヒットストップへの耐性と、ブーメランの攻撃間隔の判定に使う）
	/// </summary>
	enum class AttackType {
		kMelee,      // 近接攻撃
		kBoomerang,  // 投げたブーメラン（前にブーメランでダメージを受けてから一定の時間がたつまで当たらない。押しつぶしで浮いている間に当たれば打ち落とす）
		kFollowBoomerang,  // 追従を使った投げのブーメラン（間隔は kBoomerang と同じ。押しつぶしは打ち落とせない）
		kBurst,      // ブーメランの大技（間隔に関係なく当たり、そこから間隔を数え直す）
	};

	/// <summary>
	/// 仮モデルを作り、ステージの右寄りに置く
	/// </summary>
	/// <param name="stage">床の高さと左右の端を参照するステージ（所有しない）</param>
	/// <param name="player">狙う相手（所有しない）</param>
	void Initialize(const Stage* stage, const Player* player);

	/// <summary>
	/// 最初の状態に戻す（位置・HP・行動など。リトライで使う。モデルは作り直さず、出ている攻撃物は消す）
	/// </summary>
	void Reset();

	/// <summary>
	/// 眠らせる（チュートリアルの開始で呼ぶ）。開始時の画面に入る位置に浮かせ、盾を体の下に構える。
	/// 体の下から盾に当たった攻撃は弾き、曲げたブーメランが体かコアに当たると起きて床へ落ち、戦闘を始める（ApplySleepingAttack）
	/// </summary>
	void Sleep();

	/// <summary>
	/// 状態機械と攻撃物の更新（毎フレーム呼ぶ）
	/// </summary>
	void Update();

	/// <summary>
	/// 体・コア・盾・攻撃物を描く
	/// </summary>
	void Draw() const;

	/// <summary>
	/// 半透明の予備動作の印（突進・叩きつけの進路、落石の落下位置）を描く（不透明なものをすべて描いた後に呼ぶ）
	/// </summary>
	void DrawEffect() const;

	/// <summary>
	/// 見た目だけを更新する（止めて見せる間も毎フレーム呼ぶ。描くものは毎フレーム Update が必要なため）
	/// </summary>
	void UpdateModel();

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

	/// <summary>
	/// プレイヤーの攻撃（近接・ブーメラン）を当てる。盾が有効で正面からの攻撃なら弾き、そうでなければ 弱点 → 体 の順に調べて結果を返す
	/// </summary>
	/// <param name="attackSphere">攻撃の判定</param>
	/// <param name="damage">基本ダメージ（コアに当たると 2 倍）</param>
	/// <param name="attackerPosition">攻撃してきた側の位置（近接はプレイヤー、ブーメランはブーメラン自身）。正面か背後かの判定に使う。
	/// 近接の判定は大きく正面から体越しにコアへ届くことがあるので、判定の中心ではなく攻撃した側の位置で見る</param>
	/// <param name="type">攻撃の種類（近接を続けて受けるとヒットストップに耐性がつく。ブーメランは同じ相手へ一定の間隔でしか当たらない）</param>
	HitResult ApplyAttack(const Engine::Sphere& attackSphere, int32_t damage, const Engine::Vector3& attackerPosition, AttackType type);

	/// <summary>
	/// 眠っているボスに攻撃を当てる（チュートリアル）。体の下端より下から盾に当たった攻撃と、盾にだけ当たった攻撃は弾き、
	/// 体かコアに当たったら（盾の端に触れていても）、起こせる攻撃なら起き、そうでなければ身じろぎ（白く光って揺れる）だけする（どちらもダメージはなし）。
	/// 眠っていなければ何もしない（kNone）
	/// </summary>
	/// <param name="attackSphere">攻撃の判定</param>
	/// <param name="attackerPosition">攻撃してきた側の位置（体の下端より下から来たかの判定に使う）</param>
	/// <param name="canWakeUp">起こせる攻撃か（曲げて投げたブーメランだけ true）</param>
	HitResult ApplySleepingAttack(const Engine::Sphere& attackSphere, const Engine::Vector3& attackerPosition, bool canWakeUp);

	// 体の判定（箱）
	Engine::AABB3D GetBodyBox() const;

	// 弱点のコアの判定（背中側）
	Engine::Sphere GetCoreSphere() const;

	// 盾の位置（正面。弾いたエフェクトを出す位置）
	Engine::Vector3 GetShieldPosition() const;

	// 眠っている間の盾の判定（体の下の横長の板）
	Engine::AABB3D GetSleepShieldBox() const;

	// 盾が有効か（疲れ・怯み・やられの間は無効）
	bool IsGuarding() const;

	// 体に触れるとダメージを受ける状態か（突進で走っている間と、押しつぶしで落ちている間）
	bool IsBodyDamaging() const;

	/// <summary>
	/// 新しい攻撃を始めないようにする（プレイヤーがやられた後。今の行動は最後まで行い、終わったら待機のままになる）
	/// </summary>
	void StopAttacking();

	// このフレームに床へ着いたか（Update のたびに kNone に戻る）
	Landing GetLanding() const { return landing_; }

	// HP が半分以下か（第 2 段階以降。叩きつけのあとのビット、押しつぶしの着地のあとの落石が加わる）
	bool IsEnraged() const { return phase_ >= 2; }

	// ヒットストップに耐性がついているか（近接攻撃を続けて受けた。揺れるだけで止まらない）
	bool IsHitStopResistant() const { return meleeHitCount_ >= kHitStopResistHitCount; }

	// 眠っているか（チュートリアルの間。起こされると false になる）
	bool IsSleeping() const { return behavior_ == Behavior::kSleep; }

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

	// 押しつぶしの段階
	enum class CrushStep {
		kCrouch,   // しゃがむ（予備動作）
		kRise,     // プレイヤーの頭上へ跳び上がる
		kHover,    // 浮いたままプレイヤーを追いかける
		kLock,     // 止まって溜める（落ちる直前の合図）
		kDrop,     // 真下へ落ちる
		kRecover,  // 着地の余韻
	};

	// --- 固定値 ---
	static inline const int32_t kAttackPatternCount = 4;   // 攻撃の種類（突進 / 叩きつけ / 落石 / 押しつぶし）
	static inline const int32_t kMaxRockMarkers = 8;       // 落石の印の数の上限（RockCount もここまで）
	static inline const float kFrameTime = 1.0f / 60.0f;   // 1 フレームの秒数
	static inline const float kSleepShieldHalfThickness = 0.15f;  // 眠っている間の盾の厚みの半分

	// --- HP と数の調整値 ---
	static inline int32_t kMaxHp = 96;               // 最大 HP
	static inline int32_t kWeakPointMultiplier = 2;  // コアに当たったときのダメージ倍率
	static inline int32_t kRockCount = 4;            // 落石 1 回で落とす岩の数
	static inline int32_t kBitCount = 3;             // 第 2 段階で呼ぶビットの数

	// --- 体の調整値 ---
	static inline float kBodyHalfWidth = 1.0f;     // 体の半分の幅
	static inline float kBodyHalfHeight = 1.2f;    // 体の半分の高さ（プレイヤーのジャンプで越えられる高さにしておく）
	static inline float kShieldOffsetX = 1.15f;    // 盾の位置（中心から正面へ）
	static inline float kCoreOffsetX = 1.2f;       // コアの位置（中心から背中へ）
	static inline float kCoreOffsetY = 0.4f;       // コアの高さ（中心から上へ）
	static inline float kCoreRadius = 0.45f;       // コアの半径
	static inline float kEdgeMargin = 0.8f;        // 体の端を、ステージの左右の壁からこれだけ離す（コアが体から 0.65 はみ出すので、それより少し大きくして壁に埋まらないようにする）

	// --- 行動の調整値（秒、1 フレームあたりの量。2026-10-13 に動きを 8 割の速さにした）---
	static inline float kFirstIdleDuration = 1.9f;  // 戦闘開始から最初の攻撃までの待ち
	static inline float kIdleDuration = 1.5f;       // 攻撃と攻撃の間の待ち
	static inline float kTurnDelay = 0.6f;          // 待機中、背後に回られてから向き直るまでの遅れ
	static inline float kChargeWindup = 0.85f;      // 突進の予備動作（2026-10-27 に 0.75 から少し伸ばした）
	static inline float kChargeSpeed = 0.24f;       // 突進の速さ
	static inline float kChargeDistance = 14.0f;    // 突進で走る距離の上限（壁に着いても止まる）
	static inline float kTiredDuration = 1.9f;      // 突進後の疲れ（盾が下がる時間）
	static inline float kSlamWindup = 0.9f;         // 叩きつけの振りかぶり
	static inline float kSlamRecover = 1.0f;        // 叩きつけの余韻
	static inline float kSlamRange = 5.0f;          // 叩きつけの予備動作で、正面に示す範囲の長さ（衝撃波そのものはステージの端まで走る）
	static inline float kRockRoar = 0.6f;           // 落石の予備動作
	static inline float kRockRecover = 1.25f;       // 落石の余韻
	static inline float kRockSpawnHeight = 11.0f;   // 岩の出現する高さ
	static inline float kRockHeightStep = 2.0f;     // 岩ごとの高さの差（落ちる時間をずらす）
	static inline float kRockSpreadX = 6.0f;        // 岩を散らす横幅（プレイヤーの左右。広いほど岩と岩の間に入って避けやすい）
	static inline float kCrushCrouch = 0.5f;        // 押しつぶしの予備動作（しゃがむ時間）
	static inline float kCrushRiseDuration = 0.45f; // 押しつぶしで頭上へ跳び上がるのにかける時間
	static inline float kCrushRiseMaxDistance = 5.0f;  // 跳び上がる動作で横に進める距離の上限（プレイヤーがこれより遠いと、頭上までは届かない）
	static inline float kCrushHoverHeight = 6.0f;   // 浮く高さ（体の下端の、床からの高さ。プレイヤーのジャンプでは体に触れない高さにしておく）
	static inline float kCrushHoverDuration = 0.7f; // 浮いたままプレイヤーを追いかける時間
	static inline float kCrushTrackSpeed = 0.1f;    // 浮いている間に追いかける速さ（プレイヤーの走る速さより遅くしておく）
	static inline float kCrushLockDuration = 0.25f; // 止まって溜める時間（落ちる直前の合図）
	static inline float kCrushDropAcceleration = 0.04f;  // 落ちるときの加速度（1 フレームあたり）
	static inline float kCrushDropMaxSpeed = 0.6f;  // 落ちる速さの上限
	static inline float kCrushRecover = 0.9f;       // 着地の余韻
	static inline float kCrushRockDelay = 0.5f;     // 第 2 段階：着地してから岩を降らせるまで（その間は落下位置の印を出す。余韻より短くする）
	static inline float kShotDownFallAcceleration = 0.01f;  // 押しつぶしの途中で打ち落とされたときの落ちる加速度（1 フレームあたり。ゆっくり落とす）
	static inline float kShotDownFallMaxSpeed = 0.2f;       // 打ち落とされたときの落ちる速さの上限
	static inline float kShotDownTiltSpeed = 0.03f;         // 打ち落とされて落ちる間に体が傾いていく速さ（ラジアン / フレーム）
	static inline float kShotDownMaxTilt = 0.5f;            // 打ち落とされたときの体の傾きの上限（ラジアン。着地後の怯みの間に起き上がる）
	static inline float kHitStopDuration = 0.08f;      // 体にダメージを受けたときに止まる時間（秒）
	static inline float kCoreHitStopDuration = 0.12f;  // コアにダメージを受けたときに止まる時間（秒）
	static inline float kHitShakeAmplitude = 0.12f;    // ダメージを受けたときの揺れ幅
	static inline int32_t kHitStopResistHitCount = 3;  // 近接攻撃を続けてこの回数受けると、ヒットストップに耐性がつく（揺れるだけで止まらない）
	static inline float kBoomerangHitInterval = 0.5f;  // ブーメランのダメージが同じ相手（このボス）に入る間隔（秒。大技の一撃は間隔に関係なく入る）
	static inline float kHitStopResistResetTime = 1.0f;  // 近接攻撃の間隔がこれ以内なら「続けて」とみなす。これだけ近接を受けなければ耐性が消える（秒）
	static inline float kStaggerDuration = 0.6f;    // 怯みの時間
	static inline float kStaggerCooldown = 2.0f;    // 怯んだ後、次に怯むまでの間隔（連続で怯ませて封じられないように）
	static inline float kDeathDuration = 1.5f;      // やられ演出の時間
	static inline float kDeathSinkSpeed = 0.02f;    // やられ演出で沈む速さ
	static inline float kFlashDuration = 0.15f;     // ダメージを受けたときの白い点滅
	static inline float kShieldFlashDuration = 0.12f;  // 弾いたときの盾の白い点滅
	static inline float kBitAnchorOffsetY = 1.0f;   // ビットが回る中心の高さ（中心から上へ）
	static inline float kSleepX = -1.0f;            // 眠っている位置の X（ステージの中心から。開始時の画面に入る位置にしておく）
	static inline float kSleepHeight = 6.0f;        // 眠っている高さ（体の下端の、床からの高さ。プレイヤーのジャンプでは届かない高さにしておく）
	static inline float kSleepShieldHalfWidth = 2.5f;  // 眠っている間の盾の幅の半分（体より広げ、横から背中のコアへ届く攻撃も盾に当たるようにする）

	Engine::Object3D body_;    // 体（仮モデル：箱）
	Engine::Object3D shield_;  // 盾（仮モデル：薄い板。盾が有効なときだけ描く）
	Engine::Object3D core_;    // 弱点のコア（仮モデル：球）

	Engine::Object3D attackMark_;                                // 突進・叩きつけの進路の印（床の上の半透明の帯）
	std::array<Engine::Object3D, kMaxRockMarkers> rockMarks_;    // 落石の落下位置の印（床の上の半透明の円盤）
	bool isAttackMarkVisible_ = false;                           // 進路の印を描くか
	int32_t visibleRockMarkCount_ = 0;                           // 描く落石の印の数
	std::vector<float> rockTargetsX_;                            // 今回の落石の落下位置（吠え始めに決める）

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
	CrushStep crushStep_ = CrushStep::kCrouch;         // 押しつぶしの段階
	Engine::Vector3 crushStartPosition_ = { 0.0f, 0.0f, 0.0f };  // 跳び上がる前の位置
	float crushTargetX_ = 0.0f;                        // 跳び上がる先の X（跳ぶ瞬間のプレイヤーの位置。遠ければ進める上限まで）
	float fallSpeed_ = 0.0f;                           // 落ちる速さ（押しつぶしの落下と、空中で怯んだときの落下）
	bool isShotDown_ = false;                          // 押しつぶしの途中で打ち落とされて、床へ落ちている最中か
	bool isWakingUp_ = false;                          // 起こされて、床へ落ちている最中か
	float bodyTilt_ = 0.0f;                            // 打ち落とされたときの体の傾き（ラジアン。0 で真っすぐ）
	Landing landing_ = Landing::kNone;                 // このフレームに床へ着いたか
	int32_t attackCycle_ = 0;                          // 攻撃の順番（突進 → 叩きつけ → 落石 → …）

	int32_t hp_ = kMaxHp;                              // 残り HP
	int32_t phase_ = 1;                                // 段階（1〜3）
	bool isDeathFinished_ = false;                     // やられ演出が終わったか
	float flashTimer_ = 0.0f;                          // ダメージの白い点滅の残り時間
	float shieldFlashTimer_ = 0.0f;                    // 弾いたときの盾の点滅の残り時間
	float staggerCooldownTimer_ = 0.0f;                // 次に怯むまでの残り時間
	bool facePlayerRequest_ = false;                   // 殴られたので次の更新でプレイヤーの方を向く（当たり判定の途中で向きを変えない）
	bool isAiEnabled_ = true;                          // 開発用：攻撃に移るか
	int32_t fixedAttackIndex_ = 0;                     // 開発用：AI が選ぶ攻撃を 1 つに固定する（0 で順番どおり、1 から 突進 / 叩きつけ / 落石 / 押しつぶし）
	int32_t selectedBehaviorIndex_ = 0;                // 開発用：ImGui で選んでいる行動（kSelectableBehaviors の番号。今すぐ出す・繰り返す・調整項目の表示に使う）
	bool isLoopSelected_ = false;                      // 開発用：AI が無効のとき、選んでいる行動を繰り返すか
	int32_t meleeHitCount_ = 0;                        // 近接攻撃を続けて受けた回数（間隔が空くと 0 に戻る）
	float meleeHitTimer_ = 0.0f;                       // 前に近接攻撃を受けてからの時間（秒）
	float boomerangHitTimer_ = 0.0f;                   // 前にブーメランでダメージを受けてからの時間（秒）
	bool isHitStopFreezing_ = true;                    // 今のヒットストップで行動を止めるか（耐性があるときは揺れるだけ）
	bool isAttackEnabled_ = true;                      // 新しい攻撃を始めるか（プレイヤーがやられた後は false）
	HitStop hitStop_;                                  // ヒットストップ（ダメージを受けたとき）

	std::vector<std::unique_ptr<Hazard>> hazards_;     // 出した攻撃物（衝撃波・岩・ビット）
	std::mt19937 randomEngine_;                        // 岩の位置を散らす乱数

	// 振る舞いを切り替えたときの初期化
	void StartBehavior(Behavior next);

	// 振る舞いごとの更新
	void UpdateSleep();
	void UpdateIdle();
	void UpdateCharge();
	void UpdateTired();
	void UpdateSlam();
	void UpdateRockFall();
	void UpdateCrush();
	void UpdateStagger();
	void UpdateDead();

	// 起きる（眠っているところに上から当たった。床へ落ちて、着いてから最初の攻撃までを待つ）
	void WakeUp();

	// 次の攻撃を選ぶ（順番に出し、距離に合わないものは差し替える）
	void ChooseNextAttack();

	// 攻撃の振る舞いか（突進 / 叩きつけ / 落石 / 押しつぶし）
	static bool IsAttackBehavior(Behavior behavior);

	// 開発用：指定した振る舞いへ今すぐ切り替える（ImGui のボタンから呼ぶ。攻撃ならプレイヤーの方を向いてから始める）
	void ForceBehavior(Behavior next);

	// 開発用：ImGui で選んでいる行動
	Behavior GetSelectedBehavior() const;

#ifdef USE_IMGUI
	// 開発用：選んでいる行動に関係する調整項目だけを並べて編集する（全項目は Global Variables ウィンドウにある）
	void DrawSelectedTuningImGui();
#endif

	// ダメージを受けたときのヒットストップを始める（近接を続けて受けていたら、止めずに揺らすだけにする）
	void StartHitStop(float duration, bool isMelee);

	// 押しつぶしで浮いているところを打ち落とす（体を傾けながらゆっくり床へ落ち、着地してから怯む）
	void ShootDown();

	// コアに当たったときに怯むか（待機中・疲れ中・押しつぶしで浮いている間だけ。ほかの攻撃の途中は怯まず続ける）
	bool CanStagger() const;

	// すぐにプレイヤーの方を向く
	void FacePlayer();

	// 待機中：背後に回られたら少し遅れて向き直る
	void UpdateTurnToPlayer();

	// HP に応じて段階を進める
	void UpdatePhase();

	// 押しつぶし以外で宙にいるとき（空中で怯んだ・倒されたとき）、床まで落とす
	void UpdateFall();

	// 床に立っているときの体の中心の高さ
	float GetStandY() const;

	// 押しつぶしで自分から宙に出ている間か（跳び上がり〜落下。高さは UpdateCrush が動かす）
	bool IsCrushAirborne() const;

	// 打ち落とされたときの体の傾きを、Z 軸回りの回転にしたもの（前のめりに傾く向き。傾いていなければ 0）
	float CalcTiltRotateZ() const;

	// 攻撃物の更新と、消えたものの削除
	void UpdateHazards();

	// 見た目の位置・色を状態に合わせる
	void UpdateModels();

	// 予備動作の印の位置・濃さを状態に合わせる（予備動作中だけ描く）
	void UpdateMarks();

	void TakeDamage(int32_t damage);

	// 点が正面側（盾の側）にあるか
	bool IsInFront(const Engine::Vector3& point) const;

	// プレイヤーがいる側（+1 で右、-1 で左）
	float CalcPlayerSide() const;

	// 体の中心が動ける範囲
	float GetMinX() const;
	float GetMaxX() const;

	void SpawnShockwave();

	// 落石の落下位置を決める（吠え始めに呼び、印を出してから SpawnRocks で落とす）
	void PlanRocks();
	void SpawnRocks();
	void SpawnBits();
	void KillAllHazards();

	// 浮遊ビットがまだ残っているか（残っていれば新しく呼ばない）
	bool HasAliveBits() const;
};
