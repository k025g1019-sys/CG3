#pragma once

#include "Engine/Rendering/Sprite.h"

/// <summary>
/// ゲームオーバーの選択肢（リトライ / タイトルへ）。プレイヤーがやられた後、シーンを変えずにその場に重ねて出す。
/// A / D（または ← / →）で選び、Space / Enter で決める。左がリトライ、右がタイトルへ。
/// 文字の画像がまだ無いので、色付きの板で仮に表示している（上の赤い板が GAME OVER、緑がリトライ、青がタイトルへ。
/// 選んでいる方は明るく、白い枠が付く）。画像ができたら各スプライトのテクスチャを差し替える。
/// 位置と大きさは 1280x720 基準のピクセルで、左上の角を基準にする
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
	/// スプライトを作る（閉じた状態から始まる）
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
	/// 入力の受け付けとスプライトの更新（毎フレーム呼ぶ。閉じている間は何もしない）
	/// </summary>
	/// <param name="screenWidth">描画先の幅（px。シーンの GetRenderAreaWidth()）</param>
	/// <param name="screenHeight">描画先の高さ（px。シーンの GetRenderAreaHeight()）</param>
	void Update(float screenWidth, float screenHeight);

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

	Engine::Sprite backdrop_;     // 画面全体を少し暗くする板
	Engine::Sprite banner_;       // GAME OVER の表示の代わりの板
	Engine::Sprite cursor_;       // 選んでいる方の後ろに付く白い枠
	Engine::Sprite retryPanel_;   // リトライの表示の代わりの板（左）
	Engine::Sprite titlePanel_;   // タイトルへの表示の代わりの板（右）

	bool isOpen_ = false;             // 表示中か
	bool isDecided_ = false;          // 選択が決まったか
	Choice choice_ = Choice::kRetry;  // 今選んでいる選択肢
	float timer_ = 0.0f;              // 表示してからの時間（秒）

	// 左右の入力で選び、決定の入力で決める
	void UpdateInput();

	// 選択に合わせて板の色と枠の位置を決め、スプライトを更新する
	void UpdateSprites(float screenWidth, float screenHeight);
};
