#pragma once

#include "Engine/Rendering/Object3D.h"

#include <string>

/// <summary>
/// テキストオブジェクト（文字のモデル 1 つ）。
/// 位置・大きさ・色・上下の揺れは ImGui（GlobalVariables）で調整できる。
/// 同じグループ名を渡せば、別のシーンでも同じ調整値を使う。
/// </summary>
class TextObjects {
public:

	/// <summary>
	/// モデルを読み込み、調整項目を登録する
	/// </summary>
	/// <param name="modelPath">文字のモデル（.obj）のパス</param>
	/// <param name="groupName">調整項目のグループ名（resources/GlobalVariables/<グループ名>.csv に保存される）</param>
	/// <param name="position">位置の既定値（揺れの中心）</param>
	/// <param name="scale">大きさの既定値</param>
	/// <param name="bobAmplitude">上下の揺れ幅の既定値（0 なら揺れない）</param>
	void Initialize(const std::string& modelPath, const std::string& groupName,
		const Engine::Vector3& position, float scale, float bobAmplitude);

	/// <summary>
	/// 調整値を反映し、上下に揺らしてモデルに反映する（描くフレームは毎フレーム呼ぶ）
	/// </summary>
	void Update();

	void Draw() const;

	// 位置の基準点（調整値の Position はここからの相対位置になる。既定は原点。
	// カメラの位置を毎フレーム渡すと、画面に貼り付いたように見せられる）
	void SetOrigin(const Engine::Vector3& origin) { origin_ = origin; }

	// 明るさの倍率（調整値の色に掛ける。既定は 1。選んでいない選択肢を暗くするときに使う）
	void SetBrightness(float brightness) { brightness_ = brightness; }

private:

	/// <summary>
	/// 調整項目を適用（GlobalVariables の現在値をメンバへ反映する）
	/// </summary>
	void ApplyGlobalVariables();

	// --- 固定値 ---
	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数

	// --- 調整値（GlobalVariables で上書きされる） ---
	Engine::Vector3 position_ = { 0.0f, 0.0f, 0.0f };  // 位置（揺れの中心）
	float scale_ = 1.0f;                                // 大きさ
	Engine::Vector3 color_ = { 1.0f, 0.9f, 0.2f };     // 色（RGB）
	float bobAmplitude_ = 0.0f;                         // 上下の揺れ幅
	float bobPeriod_ = 3.0f;                            // 1 往復にかかる秒数

	Engine::Object3D object_;  // 文字のモデル
	std::string groupName_;    // 調整項目のグループ名
	float bobTimer_ = 0.0f;    // 揺れの経過時間（秒）
	Engine::Vector3 origin_ = { 0.0f, 0.0f, 0.0f };  // 位置の基準点
	float brightness_ = 1.0f;                        // 明るさの倍率
};
