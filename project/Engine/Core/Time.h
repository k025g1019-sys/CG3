#pragma once

namespace Engine {

/// <summary>
/// フレームの時間管理。更新は毎秒60回に固定する（モニターのリフレッシュレートに関係なく同じ速さで動く）。
/// 「1フレームあたりの移動量」で書いたコードはそのままでよい。
/// 秒単位で書きたいときは GetDeltaTime を掛ける（例: position.x += speedPerSecond * Engine::Time::GetDeltaTime();）。
/// </summary>
class Time {
public:

    // 目標のフレームレート（1秒あたりの更新回数）
    static constexpr float kTargetFrameRate = 60.0f;

    // 前のフレームからの経過時間（秒）。通常は約1/60秒で、処理落ちしても kMaxDeltaTime を超えない
    static float GetDeltaTime();

    // 起動してからの経過時間（秒）
    static float GetTotalTime();

    // --- 以下はFrameworkから呼ばれる ---

    // タイマーの精度を上げ、計測を開始する
    static void Initialize();

    // 1フレームの目標時間（1/60秒）が経つまで待ち、経過時間を更新する（画面表示（Present）の後に呼ぶ）
    static void WaitForNextFrame();

    // タイマーの精度設定を元に戻す
    static void Finalize();

    // 処理落ちやブレークポイントで止めた直後に物体が一気に動かないよう、経過時間の上限を決めておく
    static constexpr float kMaxDeltaTime = 0.1f;
};

} // namespace Engine
