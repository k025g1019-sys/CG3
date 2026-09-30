#include "Engine/Core/Time.h"

#include <Windows.h>
#include <timeapi.h>

#include <chrono>
#include <thread>

#pragma comment(lib, "winmm.lib")

namespace Engine {

namespace {

using Clock = std::chrono::steady_clock;

// 時間管理の状態（Timeは静的関数だけのクラスなので、状態はこのファイルの中に持つ）
struct TimeState {
	Clock::time_point startTime;       // 計測開始（起動）時刻
	Clock::time_point frameStartTime;  // 今のフレームの開始時刻
	float deltaTime = 1.0f / Time::kTargetFrameRate;
	float totalTime = 0.0f;
};

TimeState& GetState() {
	static TimeState state;
	return state;
}

}  // namespace

void Time::Initialize() {
	// Sleepの精度を1msにする（既定の約15.6msのままでは1/60秒ぴったりに合わせられない）
	timeBeginPeriod(1);

	TimeState& state = GetState();
	state.startTime = Clock::now();
	state.frameStartTime = state.startTime;
}

void Time::WaitForNextFrame() {
	TimeState& state = GetState();

	// 1フレームの目標時間（1/60秒）
	const auto kFrameTime = std::chrono::microseconds(int64_t(1000000.0f / kTargetFrameRate));
	// それより少し短い判定時間。60Hzの垂直同期などで、すでにほぼ1フレーム経っているときは待たない
	const auto kFrameCheckTime = std::chrono::microseconds(int64_t(1000000.0f / (kTargetFrameRate + 5.0f)));
	// 残りがこれより長い間はSleepで待ち、短くなったら譲りながら待って時間ぴったりに抜ける
	const auto kSleepMargin = std::chrono::milliseconds(2);

	if (Clock::now() - state.frameStartTime < kFrameCheckTime) {
		for (;;) {
			const auto elapsed = Clock::now() - state.frameStartTime;
			if (elapsed >= kFrameTime) {
				break;
			}
			if (kFrameTime - elapsed > kSleepMargin) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			} else {
				std::this_thread::yield();
			}
		}
	}

	// 経過時間を更新する（次のフレームのUpdateで使われる）
	const Clock::time_point now = Clock::now();
	float deltaTime = std::chrono::duration<float>(now - state.frameStartTime).count();
	if (deltaTime > kMaxDeltaTime) {
		deltaTime = kMaxDeltaTime;
	}
	state.deltaTime = deltaTime;
	state.totalTime = std::chrono::duration<float>(now - state.startTime).count();
	state.frameStartTime = now;
}

void Time::Finalize() {
	timeEndPeriod(1);
}

float Time::GetDeltaTime() {
	return GetState().deltaTime;
}

float Time::GetTotalTime() {
	return GetState().totalTime;
}

} // namespace Engine
