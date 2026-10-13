#pragma once

#include "Engine/Camera/Camera.h"
#include "Engine/Math/Vector3.h"

class Player;
class Stage;

/// <summary>
/// 横視点のカメラ制御。プレイヤーの X に軽く追従し、ステージの端で止まる。
/// 注目点（ボス）が渡されていれば、その方へ少し寄せてボスが画面に入りやすくする。
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

	Engine::Camera* camera_ = nullptr;        // シーンのカメラ（借りるだけ）
	const Player* target_ = nullptr;          // 追従する対象（借りるだけ）
	const Stage* stage_ = nullptr;            // 端の座標を参照するステージ（借りるだけ）

	bool hasFocus_ = false;                               // 注目点があるか
	Engine::Vector3 focusPosition_ = { 0.0f, 0.0f, 0.0f };  // 注目点（ボスの位置）

	// 画角と距離から、Z=0 平面で画面に入る幅の半分を求める
	float CalcHalfViewWidth(float aspectRatio) const;

	// ステージの外が画面に入らないよう X を制限する
	float ClampX(float x, float halfViewWidth) const;

	// 追従の目標 X
	float CalcTargetX() const;
};
