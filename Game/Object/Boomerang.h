#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"

#include <array>
#include <cstdint>

/// <summary>
/// ブーメラン。投げると目標点まで 2 次ベジェ曲線（直線もその一種）に沿って飛び、
/// 到達後は持ち主を追いかけて戻ってくる。同時に飛べるのは 1 つだけ。判定は球（Z=0 の平面なので円と同じ）。
/// 往路で地面に触れると、そこで折り返して戻ってくる。同じ敵に続けて当たれる間隔は、当たる側（ボス）が数える。
/// 飛んでいる間は、持ち主の操作で動きを変えられる。
/// ・追従：押している間カーソルを追いかけ、離すと戻ってくる。追従を使った投げは、戻ってくるまでダメージが減る
///   （半分。決めた回数だけ当てた後はさらに半分）
/// ・大技：その場で止まって大きくなり、強い一撃を 1 回だけ当てられる。少し固まった後、小さくなって速く戻る（戻りは当たらない）
/// </summary>
class Boomerang {
public:

	/// <summary>
	/// 飛行の段階
	/// </summary>
	enum class Phase {
		kIdle,          // 手元にある（飛んでいない）
		kOutward,       // 往路（目標点へ向かう）
		kReturn,        // 復路（持ち主へ戻る）
		kFollow,        // 追従（カーソルを追いかける）
		kBurst,         // 大技（その場で止まって大きくなり、強い一撃を当てる）
		kBurstRecover,  // 大技の後の硬直（止まったまま小さくなる。当たらない）
	};

	// 普通に投げたときの 1 ヒットのダメージ（調整値。追従と大技のダメージもここから決める）
	static inline int32_t kDamage = 8;

	/// <summary>
	/// 仮モデルを作る
	/// </summary>
	/// <param name="groundY">地面の高さ（これより下へは飛ばない）</param>
	void Initialize(float groundY);

	/// <summary>
	/// 手元に戻す（やり直しで最初の状態に戻すとき。モデルは作り直さない）
	/// </summary>
	void Reset();

	/// <summary>
	/// 飛行の更新（毎フレーム呼ぶ）
	/// </summary>
	/// <param name="ownerPosition">持ち主の現在位置（復路の目標）</param>
	void Update(const Engine::Vector3& ownerPosition);

	/// <summary>
	/// 見た目だけを更新する（止めて見せる間も毎フレーム呼ぶ。描くものは毎フレーム Update が必要なため）
	/// </summary>
	void UpdateModel();

	void Draw() const;

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
	/// 投げる（飛行中は無視する）
	/// </summary>
	/// <param name="start">投げ始める位置</param>
	/// <param name="target">目標点（右ボタンを離した時点のカーソル位置）</param>
	/// <param name="curveOffset">軌道の曲がり具合。始点と目標点を結ぶ線の中点から、線に垂直な方向へ曲線がどれだけ膨らむか（0 で直線、正で上側）</param>
	void Throw(const Engine::Vector3& start, const Engine::Vector3& target, float curveOffset);

	/// <summary>
	/// 盾に弾かれた。往路ならそこで戻り始め、追従中なら押し返されながら追従を続ける（ほかの段階では何もしない）
	/// </summary>
	/// <param name="pushDirection">押し返す向き（長さ 1。追従中だけ使う）</param>
	void Repel(const Engine::Vector3& pushDirection);

	/// <summary>
	/// 追従を始める（往路か、追従を使っていない復路のときだけ。CanStartFollow で確かめてから呼ぶ）
	/// </summary>
	/// <param name="target">追いかける位置（カーソルのワールド座標）</param>
	void StartFollow(const Engine::Vector3& target);

	// 追従で追いかける位置を変える（追従中は毎フレーム呼ぶ）
	void SetFollowTarget(const Engine::Vector3& target) { followTarget_ = target; }

	/// <summary>
	/// 追従をやめて戻り始める（この後は、戻ってくるまで追従も大技も出せない）
	/// </summary>
	void EndFollow();

	/// <summary>
	/// 大技を出す（追従を始めてすぐ離したとき。追従中だけ）
	/// </summary>
	void StartBurst();

	/// <summary>
	/// 往路の 2 次ベジェ曲線の制御点（始点・中間・目標点）を作る。予測線の表示にも使う。
	/// 中間の制御点は、始点と目標点を結ぶ線の上の点（CurvePeakRate の割合の位置。大きいほど目標点寄り）から、
	/// 垂直方向へ curveOffset の 2 倍ずらした点（曲線がいちばん膨らむ所でちょうど curveOffset だけ膨らむ）。
	/// 目標点寄りにすると、最初はまっすぐ気味に飛び、最後に回り込む軌道になる。
	/// 下向きに曲げたとき（curveOffset が負）は、制御点を目標点の真下へ寄せて、横へそれずに真上へ上がって目標点に入るようにする
	/// （寄せる割合は DownCurveVerticalRate。1 で目標点の真下）
	/// </summary>
	static std::array<Engine::Vector3, 3> MakeControlPoints(const Engine::Vector3& start, const Engine::Vector3& target, float curveOffset);

	bool IsFlying() const { return phase_ != Phase::kIdle; }
	bool IsFollowing() const { return phase_ == Phase::kFollow; }
	bool IsBursting() const { return phase_ == Phase::kBurst; }
	Phase GetPhase() const { return phase_; }
	const char* GetPhaseName() const;
	const Engine::Vector3& GetPosition() const { return position_; }

	// 追従を始められるか（往路か、追従を使っていない復路のとき。追従をやめた後の戻りと、大技の後はできない）
	bool CanStartFollow() const;

	// この投げで大技を出したか（キャッチした後のクールタイムに使う。次に投げるまで残る）
	bool IsBurstUsed() const { return isBurstUsed_; }

	// 今の投げで軌道を曲げたか（チュートリアルで眠っているボスを起こせるのは、曲げて投げたときだけ）
	bool IsCurvedThrow() const { return isCurvedThrow_; }

	// 当たり判定（飛行中だけ意味を持つ。大技では大きく、その後の戻りでは小さくなる）
	Engine::Sphere GetHitSphere() const { return { position_, kRadius * size_ }; }

	// 普通の大きさのときの当たり判定の半径（予測線が地面に当たる位置を求めるのに使う）
	static float GetRadius() { return kRadius; }

	// 今、敵に当たれるか（大技の後の硬直と戻りは当たらない。大技の一撃は 1 回だけ）
	bool CanHit() const;

	// 今当たったときのダメージ（大技は普通の何倍か。追従を使った投げは減らす）
	int32_t CalcDamage() const;

	// 敵にダメージを与えた（追従を使った投げの当たった回数を数える。大技の一撃はこれで終わり）
	void OnHit();

	// 盾に弾かれた演出を出してよいか（続けて出しすぎないよう間を空ける）
	bool CanShowBlockEffect() const { return blockEffectTimer_ <= 0.0f; }
	void MarkBlockEffect() { blockEffectTimer_ = kBlockEffectCooldown; }

private:

	// --- 調整値（ImGui で変更可）---
	static inline float kOutwardDuration = 0.56f;      // 往路にかける秒数
	static inline float kReturnStartSpeed = 0.064f;    // 復路の初速（1 フレームあたり）
	static inline float kReturnAcceleration = 0.0096f; // 復路の加速度（1 フレームあたり）
	static inline float kReturnMaxSpeed = 0.4f;        // 復路の最高速度（1 フレームあたり）
	static inline float kCatchDistance = 0.6f;         // 持ち主との距離がこれ以下になったら回収
	static inline float kRadius = 0.5f;                // 当たり判定の半径
	static inline float kSpinSpeed = 0.32f;            // 見た目の回転速度（ラジアン / フレーム）
	static inline float kCurvePeakRate = 0.8f;         // 曲線の山の位置（0.5 で始点と目標点の真ん中、1 に近いほど目標点寄りで最後に回り込む）
	static inline float kDownCurveVerticalRate = 1.0f; // 下向きに曲げたとき、目標点へ真下から入る度合い（0 で上向きと同じ曲げ方、1 で真上へ上がって入る）
	static inline float kBlockEffectCooldown = 0.3f;   // 盾に弾かれた演出を、次に出せるまでの時間（秒）

	// --- 追従の調整値 ---
	static inline float kFollowMaxSpeed = 0.3f;        // 追いかける速さの上限（1 フレームあたり）
	static inline float kFollowAccelTime = 0.2f;       // 動き出しで、速さを 0 から上限まで上げるのにかける時間（秒。EaseInOut）
	static inline float kFollowSlowDistance = 2.0f;    // カーソルまでの距離がこれより近いと、距離に比例して遅くする（止まり際）
	static inline float kFollowBounceSpeed = 0.25f;    // 盾に弾かれたときに押し返される初速（1 フレームあたり。0 なら押し返されない）
	static inline float kFollowBounceDuration = 0.15f; // 押し返される時間（秒。終わりにかけて遅くなる）
	static inline int32_t kFollowHalfHitCount = 2;     // 追従を使った投げで、ダメージが半分のまま当たれる回数（これより後はさらに半分）

	// --- 大技の調整値 ---
	static inline float kBurstDuration = 0.3f;         // 止まって大きくなる時間（秒。この間だけ当たる）
	static inline float kBurstScale = 3.0f;            // 大きくなったときの大きさ（普通の何倍か。判定の半径も同じ倍率）
	static inline int32_t kBurstDamageMultiplier = 3;  // 一撃のダメージ（普通の何倍か）
	static inline float kBurstRecoverDuration = 0.2f;  // 一撃の後に固まる時間（秒。この間に小さくなる）
	static inline float kBurstReturnScale = 0.5f;      // 戻るときの大きさ（普通の何倍か）
	static inline float kBurstReturnSpeedRate = 1.5f;  // 戻るときの速さ（普通の戻りの何倍か）

	// --- 固定値 ---
	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数
	static inline const float kCurvedThrowThreshold = 0.01f;  // 曲がり具合の大きさがこれより大きければ「曲げて投げた」とみなす（ホイールを戻して 0 にした誤差を除く）

	Engine::Object3D model_;                              // 見た目（仮モデル：細長い立方体を回す）
	Phase phase_ = Phase::kIdle;                          // 飛行の段階
	Engine::Vector3 position_ = { 0.0f, 0.0f, 0.0f };     // 現在位置
	float groundY_ = 0.0f;                                // 地面の高さ
	std::array<Engine::Vector3, 3> controlPoints_ = {};   // 往路の制御点（始点・中間・目標点）
	float timer_ = 0.0f;                                  // 往路・大技・大技の後の硬直の経過時間（秒）
	float speed_ = 0.0f;                                  // 復路の現在速度（1 フレームあたり）
	float size_ = 1.0f;                                   // 見た目と判定の大きさ（普通の何倍か）
	float blockEffectTimer_ = 0.0f;                       // 盾に弾かれた演出を次に出せるまでの残り時間（秒）

	Engine::Vector3 followTarget_ = { 0.0f, 0.0f, 0.0f };     // 追従で追いかける位置（カーソル）
	float followTimer_ = 0.0f;                                // 追従の動き出しからの時間（秒。押し返された後は滑り終えてから数え直す）
	Engine::Vector3 bounceDirection_ = { 0.0f, 0.0f, 0.0f };  // 盾に押し返される向き
	float bounceTimer_ = 0.0f;                                // 押し返される残り時間（秒）
	bool isFollowUsed_ = false;                               // この投げで追従を使ったか（戻ってくるまでダメージが減る）
	int32_t followHitCount_ = 0;                              // 追従を使ってから敵に当たった回数
	bool isBurstUsed_ = false;                                // この投げで大技を出したか
	bool hasBurstHit_ = false;                                // 大技の一撃を既に当てたか
	bool isCurvedThrow_ = false;                              // この投げで軌道を曲げたか

	// 往路：曲線に沿って目標点へ（終わりにかけて減速）
	void UpdateOutward();

	// 復路：持ち主へ向かって加速しながら戻る
	void UpdateReturn(const Engine::Vector3& ownerPosition);

	// 復路を始める
	void StartReturn();

	// 戻る速さの倍率（大技の後は速く戻る）
	float CalcReturnSpeedRate() const;

	// 追従：カーソルへ向かう（動き出しと止まり際をゆっくりにする）。盾に押し返されている間は、押し返される向きへ滑る
	void UpdateFollow();

	// 大技：止まったまま大きくなる
	void UpdateBurst();

	// 大技の後の硬直：止まったまま小さくなり、終わったら速く戻り始める
	void UpdateBurstRecover();

	// 地面に触れているか（触れていたら地面の上へ押し戻す）
	bool ResolveGroundCollision();
};
