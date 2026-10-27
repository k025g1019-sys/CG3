#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

/// <summary>
/// 効果音（SE）。起動時にまとめて読み込み、種類を指定して鳴らす。
/// 音量は全体と種類ごとに調整値（GlobalVariables の Sound グループ）で変えられる。
/// エンジンの Audio は同じ音を重ねて鳴らせない（鳴っている途中でもう一度鳴らすと、頭から鳴らし直す）
/// </summary>
class SoundEffects {
public:

	/// <summary>
	/// 効果音の種類
	/// </summary>
	enum class Id {
		kJump,     // ジャンプ
		kSwing,    // 近接攻撃の空振り
		kThrow,    // ブーメランを投げる
		kCatch,    // ブーメランを受け取る
		kHit,      // ボスの体・攻撃物に当たった
		kCoreHit,  // ボスのコアに当たった
		kGuard,    // 盾に弾かれた
		kDamage,   // プレイヤーが攻撃を受けた
		kLand,     // ボスが床に落ちた（押しつぶし・打ち落とし・起こされたとき）
		kDecide,   // 決定（タイトル・クリア画面・ゲームオーバーの選択肢）
		kChargeWindup,  // ボスの突進の予告
		kSlamWindup,    // ボスの叩きつけの予告
		kRoar,          // ボスの落石の予告（吠える）
		kCrushWindup,   // ボスの押しつぶしの予告
		kBurst,         // ブーメランの大技
		kPlayerDeath,   // プレイヤーのやられ
		kBossDefeat,    // ボスの撃破
		kClear,         // クリア
		kCount,    // 種類の数
	};

	// すべての効果音を読み込む（エンジンの初期化の後に一度だけ呼ぶ）
	static void Load();

	// 鳴らす（全体の音量と種類ごとの音量を掛けてから鳴らす）
	static void Play(Id id);

	// 調整項目を登録（既定値を GlobalVariables へ追加する。ファイル読み込み後に一度呼ぶ）
	static void RegisterGlobalVariables();
	// 調整項目を適用（GlobalVariables の現在値を調整項目へ反映する。毎フレーム呼ぶ）
	static void ApplyGlobalVariables();

private:

	static inline const int32_t kSoundCount = static_cast<int32_t>(Id::kCount);  // 効果音の数

	// --- 調整値 ---
	static inline float kMasterVolume = 0.8f;                     // 全体の音量
	static inline std::array<float, kSoundCount> kVolumes = {     // 種類ごとの音量（Id の順）
		0.5f, 0.5f, 0.5f, 0.6f, 0.8f, 0.8f, 0.7f, 0.8f, 1.0f, 0.6f,
		0.7f, 0.7f, 0.8f, 0.6f, 0.7f, 0.7f, 1.0f, 0.7f,
	};

	static inline std::array<size_t, kSoundCount> handles_ = {};  // エンジンの Audio のハンドル（Id の順）
	static inline bool isLoaded_ = false;                         // 読み込み済みか
};
