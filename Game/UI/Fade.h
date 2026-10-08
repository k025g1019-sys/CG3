#pragma once

#include "Engine/Rendering/Sprite.h"

/// <summary>
/// フェード。画面全体を覆う黒いスプライトを最前面に重ね、黒 → 透明（フェードイン）・透明 → 黒（フェードアウト）と変化させる。
/// シーンの開始でフェードイン、次のシーンへ移る前にフェードアウトを使う
/// </summary>
class Fade {
public:

	/// <summary>
	/// フェードの状態
	/// </summary>
	enum class Status {
		kNone,     // フェードなし
		kFadeIn,   // フェードイン中（黒 → 透明）
		kFadeOut,  // フェードアウト中（透明 → 黒）
	};

	/// <summary>
	/// 画面全体を覆う黒いスプライトを作る
	/// </summary>
	void Initialize();

	/// <summary>
	/// 経過時間を進めて不透明度を更新する（毎フレーム呼ぶ）
	/// </summary>
	void Update();

	/// <summary>
	/// フェード中だけスプライトを描く（OnDraw の一番最後に呼ぶ）
	/// </summary>
	void Draw() const;

	/// <summary>
	/// フェードを始める
	/// </summary>
	/// <param name="status">フェードイン か フェードアウト</param>
	/// <param name="duration">かける時間（秒）</param>
	void Start(Status status, float duration);

	/// <summary>
	/// フェードを止めて状態を kNone に戻す（フェードインが終わったら呼び、スプライトを描かないようにする）
	/// </summary>
	void Stop();

	/// <summary>
	/// 今のフェードが終わったか（フェードなしのときは true）
	/// </summary>
	bool IsFinished() const;

	Status GetStatus() const { return status_; }

private:

	static inline const float kFrameTime = 1.0f / 60.0f;  // 1 フレームの秒数

	Engine::Sprite sprite_;          // 画面全体を覆う黒い板
	Status status_ = Status::kNone;  // 現在の状態
	float duration_ = 0.0f;          // フェードにかける時間（秒）
	float counter_ = 0.0f;           // 経過時間（秒）
};
