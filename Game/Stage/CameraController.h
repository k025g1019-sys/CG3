#pragma once

#include "Engine/Camera/Camera.h"

class Player;
class Stage;

/// <summary>
/// 横視点のカメラ制御。プレイヤーの X に軽く追従し、ステージの端で止まる。
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

#ifdef USE_IMGUI
	/// <summary>
	/// 距離や追従率を編集する（呼び出し側の ImGui ウィンドウの中に差し込む）
	/// </summary>
	void DrawImGui();
#endif

private:

	// --- 調整値（ImGui で変更可）---
	static inline float kDistance = 22.0f;    // ゲーム平面（Z=0）からカメラまでの距離。大きいほど広く映る
	static inline float kHeight = 4.0f;       // カメラの高さ（Y）
	static inline float kFollowRate = 0.1f;   // 追従の補間率（1 で即座に追いつく）
	static inline float kOffsetX = 0.0f;      // 目標位置を対象からずらす量

	Engine::Camera* camera_ = nullptr;        // シーンのカメラ（借りるだけ）
	const Player* target_ = nullptr;          // 追従する対象（借りるだけ）
	const Stage* stage_ = nullptr;            // 端の座標を参照するステージ（借りるだけ）

	// 画角と距離から、Z=0 平面で画面に入る幅の半分を求める
	float CalcHalfViewWidth(float aspectRatio) const;

	// ステージの外が画面に入らないよう X を制限する
	float ClampX(float x, float halfViewWidth) const;

	// 追従の目標 X
	float CalcTargetX() const;
};
