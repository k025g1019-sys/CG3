#pragma once

#include "Engine/Rendering/Object3D.h"

/// <summary>
/// ボス戦の場（床と左右の端）。床の高さと左右の端の座標を持ち、床と壁の仮モデルを描く。
/// ゲームは Z=0 の平面で進み、+X が右、+Y が上。
/// </summary>
class Stage {
public:

	/// <summary>
	/// 床と壁の仮モデルを作る
	/// </summary>
	void Initialize();

	/// <summary>
	/// 幅の変更をモデルに反映する（毎フレーム呼ぶ）
	/// </summary>
	void Update();

	void Draw() const;

#ifdef USE_IMGUI
	/// <summary>
	/// 幅などを編集する（呼び出し側の ImGui ウィンドウの中に差し込む）
	/// </summary>
	void DrawImGui();
#endif

	// 床の上面の高さ
	float GetGroundY() const { return kGroundY; }

	// 左端・右端の X 座標（この内側が動ける範囲）
	float GetLeftEdge() const { return -kWidth * 0.5f; }
	float GetRightEdge() const { return kWidth * 0.5f; }

	float GetWidth() const { return kWidth; }

private:

	// --- 調整値（ImGui で変更可）---
	static inline float kWidth = 32.0f;                 // 横幅（1.8 画面分の目安）

	// --- 固定値 ---
	static inline const float kGroundY = 0.0f;          // 床の上面の高さ
	static inline const float kFloorThickness = 1.0f;   // 床の厚み（上面が kGroundY になるよう下へ伸ばす）
	static inline const float kFloorDepth = 8.0f;       // 床の奥行き（見た目用）
	static inline const float kWallThickness = 1.0f;    // 左右の壁の厚み
	static inline const float kWallHeight = 14.0f;      // 左右の壁の高さ

	Engine::Object3D floor_;      // 床（仮モデル：横に伸ばした立方体）
	Engine::Object3D leftWall_;   // 左の壁（仮モデル）
	Engine::Object3D rightWall_;  // 右の壁（仮モデル）

	// 幅に合わせて床と壁の位置・大きさを決める
	void ApplyLayout();
};
