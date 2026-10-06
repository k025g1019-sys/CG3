#include "Engine/Input/Input.h"

#include <cassert>
#include <cmath>    // sqrtf
#include <cstring>  // std::memcpy, std::memset

#include "Engine/Core/Time.h"

// DirectInput関連ライブラリ（dxguid.libはGUID_SysKeyboard等のため）
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
// ゲームパッド入力・振動（XInput）
#pragma comment(lib, "xinput.lib")

namespace Engine {

Input* Input::GetInstance() {
    static Input instance;
    return &instance;
}

void Input::Initialize(HWND hwnd) {
    HRESULT hr;

    hwnd_ = hwnd;  // カーソル座標の変換で使う

    // DirectInput本体を生成
    hr = DirectInput8Create(
        GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8,
        reinterpret_cast<void**>(directInput_.GetAddressOf()), nullptr);
    assert(SUCCEEDED(hr));

    // キーボードデバイスを生成
    hr = directInput_->CreateDevice(GUID_SysKeyboard, keyboard_.GetAddressOf(), nullptr);
    assert(SUCCEEDED(hr));

    // 入力データ形式を標準キーボードに設定
    hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    // 排他制御レベルを設定（フォアグラウンド時のみ・非排他）
    hr = keyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    assert(SUCCEEDED(hr));

    // マウスデバイスを生成
    hr = directInput_->CreateDevice(GUID_SysMouse, mouse_.GetAddressOf(), nullptr);
    assert(SUCCEEDED(hr));

    // 入力データ形式をマウス（ボタン8個・ホイール対応）に設定
    hr = mouse_->SetDataFormat(&c_dfDIMouse2);
    assert(SUCCEEDED(hr));

    // 排他制御レベルを設定（フォアグラウンド時のみ・非排他）
    hr = mouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    assert(SUCCEEDED(hr));

    // ゲームパッド（XInput）はデバイスの生成が不要なので、ここでは何もしない
}

void Input::Finalize() {
    // 終了後にコントローラーが振動し続けないよう、全プレイヤーの振動を停止
    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        XINPUT_VIBRATION zero = {};
        XInputSetState(static_cast<DWORD>(i), &zero);
    }

    if (mouse_) {
        mouse_->Unacquire();
    }
    mouse_.Reset();
    if (keyboard_) {
        keyboard_->Unacquire();
    }
    keyboard_.Reset();
    directInput_.Reset();
}

void Input::Update() {
    // 前フレームの状態を退避（エッジ検出用）
    std::memcpy(keyPre_, key_, sizeof(key_));
    mouseStatePre_ = mouseState_;

    // 全キーの入力状態をまとめて取得する（Acquireはフォーカス喪失からの復帰のため毎フレーム呼ぶ）。
    // 取得できないとき（ウィンドウが前面にない等）はゼロクリアし、押しっぱなし扱いが残らないようにする
    if (FAILED(keyboard_->Acquire()) ||
        FAILED(keyboard_->GetDeviceState(static_cast<DWORD>(sizeof(key_)), key_))) {
        std::memset(key_, 0, sizeof(key_));
    }

    // マウスの状態を取得（フォーカス喪失時は移動量が残らないようゼロクリアする）
    if (FAILED(mouse_->Acquire()) ||
        FAILED(mouse_->GetDeviceState(static_cast<DWORD>(sizeof(DIMOUSESTATE2)), &mouseState_))) {
        mouseState_ = {};
    }

    // --- ゲームパッド（XInput） ---
    // 振動の自動停止に使う経過時間（エンジンの時間管理から取得。処理落ち時の上限もTime側で適用済み）
    const float deltaTime = Time::GetDeltaTime();

    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        Pad& pad = pads_[i];

        // 前フレームの状態を退避して最新の状態を取得する
        pad.statePre = pad.state;
        pad.connected = (XInputGetState(static_cast<DWORD>(i), &pad.state) == ERROR_SUCCESS);
        if (!pad.connected) {
            pad.state = {};  // 未接続はゼロ化（ボタンが押下扱いにならないように）
        }

        // 振動の自動停止タイマーを進める
        if (pad.vibrationTimer > 0.0f) {
            pad.vibrationTimer -= deltaTime;
            if (pad.vibrationTimer <= 0.0f) {
                pad.vibrationTimer = 0.0f;
                ApplyVibration(i, 0.0f, 0.0f);  // 時間切れで停止
            }
        }
    }
}

bool Input::IsPress(uint8_t keyNumber) const {
    return (key_[keyNumber] & 0x80) != 0;
}

bool Input::IsTrigger(uint8_t keyNumber) const {
    // 今フレームは押下、前フレームは非押下
    return (key_[keyNumber] & 0x80) != 0 && (keyPre_[keyNumber] & 0x80) == 0;
}

bool Input::IsRelease(uint8_t keyNumber) const {
    // 今フレームは非押下、前フレームは押下
    return (key_[keyNumber] & 0x80) == 0 && (keyPre_[keyNumber] & 0x80) != 0;
}

bool Input::IsMousePress(int button) const {
    if (!IsValidMouseButton(button)) {
        return false;
    }
    return (mouseState_.rgbButtons[button] & 0x80) != 0;
}

bool Input::IsMouseTrigger(int button) const {
    if (!IsValidMouseButton(button)) {
        return false;
    }
    // 今フレームは押下、前フレームは非押下
    return (mouseState_.rgbButtons[button] & 0x80) != 0 && (mouseStatePre_.rgbButtons[button] & 0x80) == 0;
}

bool Input::IsMouseRelease(int button) const {
    if (!IsValidMouseButton(button)) {
        return false;
    }
    // 今フレームは非押下、前フレームは押下
    return (mouseState_.rgbButtons[button] & 0x80) == 0 && (mouseStatePre_.rgbButtons[button] & 0x80) != 0;
}

Vector2 Input::GetMouseMove() const {
    return { static_cast<float>(mouseState_.lX), static_cast<float>(mouseState_.lY) };
}

float Input::GetWheel() const {
    return static_cast<float>(mouseState_.lZ);
}

Vector2 Input::GetMousePosition() const {
    POINT point = {};
    GetCursorPos(&point);
    ScreenToClient(hwnd_, &point);
    return { static_cast<float>(point.x), static_cast<float>(point.y) };
}

// --- ゲームパッド（XInput） ---

bool Input::IsPadConnected(int playerIndex) const {
    if (!IsValidPlayerIndex(playerIndex)) {
        return false;
    }
    return pads_[playerIndex].connected;
}

bool Input::IsPadPress(int button, int playerIndex) const {
    if (!IsValidPlayerIndex(playerIndex)) {
        return false;
    }
    return (pads_[playerIndex].state.Gamepad.wButtons & button) != 0;
}

bool Input::IsPadTrigger(int button, int playerIndex) const {
    if (!IsValidPlayerIndex(playerIndex)) {
        return false;
    }
    // 今フレームは押下、前フレームは非押下
    const Pad& pad = pads_[playerIndex];
    bool isPress = (pad.state.Gamepad.wButtons & button) != 0;
    bool wasPress = (pad.statePre.Gamepad.wButtons & button) != 0;
    return isPress && !wasPress;
}

bool Input::IsPadRelease(int button, int playerIndex) const {
    if (!IsValidPlayerIndex(playerIndex)) {
        return false;
    }
    // 今フレームは非押下、前フレームは押下
    const Pad& pad = pads_[playerIndex];
    bool isPress = (pad.state.Gamepad.wButtons & button) != 0;
    bool wasPress = (pad.statePre.Gamepad.wButtons & button) != 0;
    return !isPress && wasPress;
}

float Input::GetLeftTrigger(int playerIndex) const {
    if (!IsValidPlayerIndex(playerIndex)) {
        return 0.0f;
    }
    return ApplyTriggerDeadzone(pads_[playerIndex].state.Gamepad.bLeftTrigger);
}

float Input::GetRightTrigger(int playerIndex) const {
    if (!IsValidPlayerIndex(playerIndex)) {
        return 0.0f;
    }
    return ApplyTriggerDeadzone(pads_[playerIndex].state.Gamepad.bRightTrigger);
}

Vector2 Input::GetLeftStick(int playerIndex) const {
    if (!IsValidPlayerIndex(playerIndex)) {
        return { 0.0f, 0.0f };
    }
    const XINPUT_GAMEPAD& gamepad = pads_[playerIndex].state.Gamepad;
    return ApplyStickDeadzone(
        gamepad.sThumbLX,
        gamepad.sThumbLY,
        static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

Vector2 Input::GetRightStick(int playerIndex) const {
    if (!IsValidPlayerIndex(playerIndex)) {
        return { 0.0f, 0.0f };
    }
    const XINPUT_GAMEPAD& gamepad = pads_[playerIndex].state.Gamepad;
    return ApplyStickDeadzone(
        gamepad.sThumbRX,
        gamepad.sThumbRY,
        static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}

void Input::SetVibration(float leftMotor, float rightMotor, int playerIndex) {
    if (!IsValidPlayerIndex(playerIndex)) {
        return;
    }
    pads_[playerIndex].vibrationTimer = 0.0f;  // 自動停止なし（StopVibrationまで持続）
    ApplyVibration(playerIndex, leftMotor, rightMotor);
}

void Input::SetVibrationForTime(float leftMotor, float rightMotor, float seconds, int playerIndex) {
    if (!IsValidPlayerIndex(playerIndex)) {
        return;
    }
    if (seconds <= 0.0f) {
        StopVibration(playerIndex);  // 0以下は即停止
        return;
    }
    pads_[playerIndex].vibrationTimer = seconds;
    ApplyVibration(playerIndex, leftMotor, rightMotor);
}

void Input::StopVibration(int playerIndex) {
    if (!IsValidPlayerIndex(playerIndex)) {
        return;
    }
    pads_[playerIndex].vibrationTimer = 0.0f;
    ApplyVibration(playerIndex, 0.0f, 0.0f);
}

bool Input::IsValidPlayerIndex(int playerIndex) {
    return playerIndex >= 0 && playerIndex < XUSER_MAX_COUNT;
}

bool Input::IsValidMouseButton(int button) const {
    // ボタン数はDIMOUSESTATE2のrgbButtonsの要素数（8個）
    return button >= 0 && button < static_cast<int>(_countof(mouseState_.rgbButtons));
}

float Input::ApplyTriggerDeadzone(BYTE value) {
    // XInput推奨の閾値未満は「押していない」とみなす
    // （トリガーが完全に戻らない個体で、無入力なのに拡縮し続けるような誤反応を防ぐ）
    if (value < XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
        return 0.0f;
    }
    // 閾値から最大値までを0.0〜1.0へ再スケールする（浅い踏み込みでも滑らかに立ち上がる）
    return static_cast<float>(value - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
           static_cast<float>(255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

Vector2 Input::ApplyStickDeadzone(short x, short y, float deadzone) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
    float magnitude = sqrtf(fx * fx + fy * fy);

    // デッドゾーン内は入力なしとみなす
    if (magnitude <= deadzone) {
        return { 0.0f, 0.0f };
    }

    // 最大値でクランプ（明示比較：Windows.hのmin/maxマクロ回避）
    const float kMax = 32767.0f;
    float clamped = magnitude > kMax ? kMax : magnitude;

    // デッドゾーンを差し引いて0.0〜1.0へ再スケールし、方向ベクトルに乗じる
    float normalized = (clamped - deadzone) / (kMax - deadzone);
    return { (fx / magnitude) * normalized, (fy / magnitude) * normalized };
}

void Input::ApplyVibration(int playerIndex, float leftMotor, float rightMotor) {
    // 0.0〜1.0にクランプ（明示比較：Windows.hのmin/maxマクロ回避）
    if (leftMotor < 0.0f) { leftMotor = 0.0f; }
    if (leftMotor > 1.0f) { leftMotor = 1.0f; }
    if (rightMotor < 0.0f) { rightMotor = 0.0f; }
    if (rightMotor > 1.0f) { rightMotor = 1.0f; }

    XINPUT_VIBRATION vibration = {};
    vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
    XInputSetState(static_cast<DWORD>(playerIndex), &vibration);
}

} // namespace Engine
