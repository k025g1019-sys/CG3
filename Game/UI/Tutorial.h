#pragma once

#include "Engine/Math/Vector3.h"
#include "Game/Object/TextObjects.h"

class Player;

/// <summary>
/// チュートリアルの説明文（ボス戦の最初の段階で出す）。
/// 3 つのブロック（移動とジャンプ → 近接と遠距離攻撃 → ブーメランを曲げる）を順に出し、
/// ブロックの中の行動をすべてしたら、少し待って次のブロックへ切り替える。
/// 最後のブロックはボスを起こすまで出しておく（ボスが起きたかはシーンが見て、チュートリアルを終える）。
/// 文字は 3D のモデル（TextObjects）で、カメラの少し前に毎フレーム置き直して画面に貼り付いたように見せる。
/// 位置・大きさ・色は ImGui の TutorialMove / TutorialAttack / TutorialCurve で調整する（Position はカメラからの相対位置。z がカメラからの距離）
/// </summary>
class Tutorial {
public:

	/// <summary>
	/// 文字のモデルを作り、最初のブロックから始める
	/// </summary>
	/// <param name="player">行動を見る相手（所有しない）</param>
	void Initialize(const Player* player);

	/// <summary>
	/// プレイヤーの行動を見てブロックを進め、文字をカメラの前に置き直す（カメラを動かした後に毎フレーム呼ぶ）
	/// </summary>
	/// <param name="cameraPosition">カメラの位置（文字の位置の基準点）</param>
	void Update(const Engine::Vector3& cameraPosition);

	/// <summary>
	/// 今のブロックの文字を描く
	/// </summary>
	void Draw() const;

	// 今のブロックの名前（ImGui の表示用）
	const char* GetStepName() const;

private:

	/// <summary>
	/// ブロック（出す説明文）
	/// </summary>
	enum class Step {
		kMove,    // A、D で移動 ／ W かスペースでジャンプ
		kAttack,  // マウス左で近接攻撃 ／ 右でカーソル位置に遠距離攻撃
		kCurve,   // 武器を投げていない状態で、マウス右を押したままホイールを回す（ボスを起こすまで出しておく）
	};

	// --- 固定値 ---
	static inline const float kFrameTime = 1.0f / 60.0f;      // 1 フレームの秒数
	static inline const float kNextStepDelay = 0.8f;          // ブロックの行動をすべてしてから、次のブロックへ切り替えるまでの時間（秒）
	static inline const float kMoveSpeedThreshold = 0.01f;    // 横の速さがこれより大きければ「その向きへ移動した」とみなす

	TextObjects moveText_;    // 移動とジャンプの説明
	TextObjects attackText_;  // 近接と遠距離攻撃の説明
	TextObjects curveText_;   // ブーメランを曲げる説明

	const Player* player_ = nullptr;  // 行動を見る相手（借りるだけ）

	Step step_ = Step::kMove;         // 今のブロック
	float clearTimer_ = 0.0f;         // ブロックの行動をすべてしてからの時間（秒）
	bool hasMovedLeft_ = false;       // 左へ移動したか
	bool hasMovedRight_ = false;      // 右へ移動したか
	bool hasJumped_ = false;          // ジャンプしたか
	bool hasMeleeAttacked_ = false;   // 近接攻撃をしたか
	bool hasThrown_ = false;          // ブーメランを投げたか

	// プレイヤーの今の状態から、した行動を覚える
	void CheckActions();

	// 今のブロックの行動をすべてしたか（最後のブロックは、ボスを起こすまで終わらない）
	bool IsStepCleared() const;

	// 今のブロックの文字
	TextObjects& GetCurrentText();
	const TextObjects& GetCurrentText() const;
};
