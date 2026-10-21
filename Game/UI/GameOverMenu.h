#pragma once

#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Sprite.h"
#include "Game/Object/TextObjects.h"

/// <summary>
/// ゲームオーバーの選択肢（リトライ / タイトルへ）。プレイヤーがやられた後、シーンを変えずにその場に重ねて出す。
/// A / D（または ← / →）で選び、Space / Enter で決める。左がリトライ、右がタイトルへ。
/// 文字は 3D のモデル（TextObjects）で、カメラの少し前に毎フレーム置き直して画面に貼り付いたように見せる。
/// 選んでいる方は元の色、選んでいない方は暗くする。位置・大きさ・色は ImGui の
/// GameOverText / RetryText / ReturnToTitleText で調整する（Position はカメラからの相対位置。z がカメラからの距離）。
/// 画面を暗くする板はスプライトで、深度を奥（ステージより手前、文字より奥）にずらして文字を隠さないようにしている
/// </summary>
class GameOverMenu {
public:

	/// <summary>
	/// 選択肢
	/// </summary>
	enum class Choice {
		kRetry,  // リトライ（その場ですぐ再挑戦）
		kTitle,  // タイトルへ戻る
	};

	/// <summary>
	/// スプライトと文字のモデルを作る（閉じた状態から始まる）
	/// </summary>
	void Initialize();

	/// <summary>
	/// 表示を始める（リトライを選んだ状態から。押しっぱなしの入力で決まらないよう、少しの間は入力を受け付けない）
	/// </summary>
	void Open();

	/// <summary>
	/// 表示をやめる
	/// </summary>
	void Close();

	/// <summary>
	/// 入力の受け付けと、スプライト・文字の更新（毎フレーム呼ぶ。閉じている間は何もしない）。
	/// 文字がカメラから遅れないよう、カメラを動かした後に呼ぶ
	/// </summary>
	/// <param name="screenWidth">描画先の幅（px。シーンの GetRenderAreaWidth()）</param>
	/// <param name="screenHeight">描画先の高さ（px。シーンの GetRenderAreaHeight()）</param>
	/// <param name="cameraPosition">このフレームのカメラの位置（文字を置く基準）</param>
	void Update(float screenWidth, float screenHeight, const Engine::Vector3& cameraPosition);

	/// <summary>
	/// 開いている間だけ描く（OnDraw の最後、フェードの手前で呼ぶ）
	/// </summary>
	void Draw() const;

	// 表示中か
	bool IsOpen() const { return isOpen_; }

	// 選択が決まったか（決まった後は入力を受け付けない）
	bool IsDecided() const { return isDecided_; }

	// 今選んでいる（決まった後は、決めた）選択肢
	Choice GetChoice() const { return choice_; }
	const char* GetChoiceName() const;

private:

	static inline const float kInputDelay = 0.3f;         // 表示してから入力を受け付けるまでの時間（秒）
	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数

	Engine::Sprite backdrop_;        // 画面全体を少し暗くする板
	TextObjects gameOverText_;       // GAME OVER の文字
	TextObjects retryText_;          // リトライの文字（左）
	TextObjects returnToTitleText_;  // タイトルへの文字（右）

	bool isOpen_ = false;             // 表示中か
	bool isDecided_ = false;          // 選択が決まったか
	Choice choice_ = Choice::kRetry;  // 今選んでいる選択肢
	float timer_ = 0.0f;              // 表示してからの時間（秒）

	// 左右の入力で選び、決定の入力で決める
	void UpdateInput();

	// 選択に合わせて文字の明るさを決め、スプライトと文字を更新する
	void UpdateDisplay(float screenWidth, float screenHeight, const Engine::Vector3& cameraPosition);
};
