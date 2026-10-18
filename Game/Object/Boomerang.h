#pragma once

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Object3D.h"

#include <array>
#include <cstdint>

/// <summary>
/// ブーメラン。投げると目標点まで 2 次ベジェ曲線（直線もその一種）に沿って飛び、
/// 到達後は持ち主を追いかけて戻ってくる。同時に飛べるのは 1 つだけ。判定は球（Z=0 の平面なので円と同じ）。
/// 往路と復路でそれぞれ 1 回ずつ敵に当たれる。往路で地面に触れると、そこで折り返して戻ってくる。
/// </summary>
class Boomerang {
public:

	/// <summary>
	/// 飛行の段階
	/// </summary>
	enum class Phase {
		kIdle,     // 手元にある（飛んでいない）
		kOutward,  // 往路（目標点へ向かう）
		kReturn,   // 復路（持ち主へ戻る）
	};

	// 1 ヒットのダメージ（往路と復路で同じ。調整値）
	static inline int32_t kDamage = 2;

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
	/// 弾かれて戻る（盾に当たったときなど。往路の途中でも復路に切り替える）
	/// </summary>
	void Repel();

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
	Phase GetPhase() const { return phase_; }
	const Engine::Vector3& GetPosition() const { return position_; }

	// 当たり判定（飛行中だけ意味を持つ）
	Engine::Sphere GetHitSphere() const { return { position_, kRadius }; }

	// 当たり判定の半径（予測線が地面に当たる位置を求めるのに使う）
	static float GetRadius() { return kRadius; }

	// この段階（往路 / 復路）でまだ当たっていないか
	bool CanHit() const { return IsFlying() && !hasHit_; }
	void MarkHit() { hasHit_ = true; }

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

	// --- 固定値 ---
	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数

	Engine::Object3D model_;                              // 見た目（仮モデル：細長い立方体を回す）
	Phase phase_ = Phase::kIdle;                          // 飛行の段階
	Engine::Vector3 position_ = { 0.0f, 0.0f, 0.0f };     // 現在位置
	float groundY_ = 0.0f;                                // 地面の高さ
	std::array<Engine::Vector3, 3> controlPoints_ = {};   // 往路の制御点（始点・中間・目標点）
	float timer_ = 0.0f;                                  // 往路の経過時間（秒）
	float speed_ = 0.0f;                                  // 復路の現在速度（1 フレームあたり）
	bool hasHit_ = false;                                 // 今の段階で既に何かに当たったか

	// 往路：曲線に沿って目標点へ（終わりにかけて減速）
	void UpdateOutward();

	// 復路：持ち主へ向かって加速しながら戻る
	void UpdateReturn(const Engine::Vector3& ownerPosition);

	// 復路を始める
	void StartReturn();

	// 地面に触れているか（触れていたら地面の上へ押し戻す）
	bool ResolveGroundCollision();
};
