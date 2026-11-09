#pragma once

#include "Engine/Rendering/Object3D.h"

/// <summary>
/// 天球（背景の空）。ステージを囲む大きな球で、内側に空の画像が貼ってある。
/// モデルの面は内側を向いているので、ふつうの描画（裏面は描かない）のまま内側から見える。
/// 光の向きで暗くならないよう、陰影なし（画像の色そのまま）で描く。
/// </summary>
class Skydome {
public:

	/// <summary>
	/// 天球のモデルを読み込み、ステージの中心に置く
	/// </summary>
	void Initialize();

	/// <summary>
	/// モデルの位置・大きさを反映する（描くフレームは毎フレーム呼ぶ）
	/// </summary>
	void Update();

	void Draw() const;

private:

	// --- 固定値 ---
	static inline const float kRadius = 50.0f;  // 半径（モデルは半径 1。カメラの描画距離 100 に収まる大きさ）

	Engine::Object3D object_;  // 天球のモデル（resources/skydome/skydome.obj）
};
