#pragma once

#include "Engine/Camera/Camera.h"
#include "Engine/Math/Vector3.h"

class Player;
class Stage;

/// <summary>
/// 横視点のカメラ制御。プレイヤーの X に軽く追従し、ステージの端で止まる。
/// 注目点（ボス）が渡されていれば、その方へ少し寄せてボスが画面に入りやすくする。
/// プレイヤーがボスから離れていくとき（と止まっているとき）は、ボスが画面に入るところまでカメラを寄せる（カメラの距離は変えず、横移動だけ）。
/// 離れすぎて両方は映せないときは、プレイヤーが画面の端に寄りすぎないことを優先し、ボスが画面の外へ出る。
/// ボスへ向かって動いている間は、この補正をしない（プレイヤーを追い、ボスの方へ少し寄せるだけ）。
/// Y と Z は固定で、回転 0（+Z 向き）のまま使う。
/// </summary>
class CameraController {
public:

	/// <summary>
	/// 追従する対象とステージを受け取る（カメラはシーンのものを借りる。所有しない）
	/// </summary>
	void Initialize(Engine::Camera* camera, const Player* target, const Stage* stage);

	/// <summary>
	/// 補間で追従する（毎フレーム呼ぶ）
	/// </summary>
	/// <param name="aspectRatio">描画先のアスペクト比（画面端の計算に使う）</param>
	void Update(float aspectRatio);

	/// <summary>
	/// 補間なしで目標位置へ合わせる（シーン開始時など）
	/// </summary>
	void Reset(float aspectRatio);

	/// <summary>
	/// 注目点を渡す（ボスの位置。Update の前に毎フレーム呼ぶ）。カメラをプレイヤーからこの方へ少し寄せる
	/// </summary>
	void SetFocusPosition(const Engine::Vector3& position);

	/// <summary>
	/// 注目点をやめる（プレイヤーだけを追う）
	/// </summary>
	void ClearFocus() { hasFocus_ = false; }

	/// <summary>
	/// カメラを揺らす（ボスが床に落ちたときなど）。揺れは終わりにかけて小さくなる。揺れている途中で呼ぶと始め直す
	/// </summary>
	/// <param name="amplitude">揺れ幅（ワールドの長さ）</param>
	/// <param name="duration">揺れる時間（秒）</param>
	void StartShake(float amplitude, float duration);

#ifdef USE_IMGUI
	/// <summary>
	/// カメラの位置を表示する（呼び出し側の ImGui ウィンドウの中に差し込む。調整値の編集は Global Variables ウィンドウで行う）
	/// </summary>
	void DrawImGui();
#endif

	// 調整項目を登録（既定値を GlobalVariables へ追加する。ファイル読み込み後に一度呼ぶ）
	static void RegisterGlobalVariables();
	// 調整項目を適用（GlobalVariables の現在値を調整項目へ反映する。毎フレーム呼ぶ）
	static void ApplyGlobalVariables();

private:

	// --- 調整値（ImGui で変更可）---
	static inline float kDistance = 22.0f;    // ゲーム平面（Z=0）からカメラまでの距離。大きいほど広く映る
	static inline float kHeight = 4.0f;       // カメラの高さ（Y）
	static inline float kFollowRate = 0.1f;   // 追従の補間率（1 で即座に追いつく）
	static inline float kOffsetX = 0.0f;      // 目標位置を対象からずらす量
	static inline float kFocusRate = 0.3f;    // 注目点の方へ寄せる割合（0 で対象だけ、1 で注目点）
	static inline float kMaxFocusOffset = 5.0f;  // 注目点の方へ寄せる量の上限（対象が画面から外れないように）
	static inline float kFocusMargin = 2.0f;     // 注目点（ボスの中心）を、画面の端からこれだけ内側に映す（体の半分の幅より大きくしておく。
	                                             // 対象が注目点から離れていくときと、止まっているときだけ働く）
	static inline float kTargetMargin = 3.0f;    // 対象（プレイヤー）を、画面の端から最低これだけ内側に保つ（注目点を映すことより優先する）

	// --- 固定値 ---
	static inline const float kApproachSpeedThreshold = 0.01f;  // 対象が注目点へ向かう速さがこれより大きい間は「近づいている」とみなす
	static inline const float kFrameTime = 1.0f / 60.0f;        // 1 フレームの秒数
	static inline const float kShakeSpeedX = 70.0f;             // 揺れの速さ（横。ラジアン / 秒）
	static inline const float kShakeSpeedY = 55.0f;             // 揺れの速さ（縦。横と少しずらして単調に見えないようにする）

	Engine::Camera* camera_ = nullptr;        // シーンのカメラ（借りるだけ）
	const Player* target_ = nullptr;          // 追従する対象（借りるだけ）
	const Stage* stage_ = nullptr;            // 端の座標を参照するステージ（借りるだけ）

	float currentX_ = 0.0f;                               // 追従している X（揺れを足す前の位置。補間はこの値から行う）
	float shakeTimer_ = 0.0f;                             // 揺れの残り時間（秒）
	float shakeDuration_ = 0.0f;                          // 今回揺らす時間（秒）
	float shakeAmplitude_ = 0.0f;                         // 今回の揺れ幅

	bool hasFocus_ = false;                               // 注目点があるか
	Engine::Vector3 focusPosition_ = { 0.0f, 0.0f, 0.0f };  // 注目点（ボスの位置）

	// 画角と距離から、Z=0 平面で画面に入る幅の半分を求める
	float CalcHalfViewWidth(float aspectRatio) const;

	// ステージの外が画面に入らないよう X を制限する
	float ClampX(float x, float halfViewWidth) const;

	// 対象（プレイヤー）が画面の端に寄りすぎないよう X を制限する
	float ClampToKeepTarget(float x, float halfViewWidth) const;

	// 追従の目標 X
	float CalcTargetX(float halfViewWidth) const;

	// 対象が注目点へ向かって動いているか
	bool IsTargetApproachingFocus() const;

	// 揺れの時間を進め、今の揺れの量を返す（揺れていなければ 0）
	Engine::Vector3 UpdateShake();
};
