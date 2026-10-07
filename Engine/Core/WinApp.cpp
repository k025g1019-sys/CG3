#include "Engine/Core/WinApp.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM LParam);
#endif

namespace Engine {

LRESULT CALLBACK WinApp::WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
) {
#ifdef USE_IMGUI
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return TRUE;
    }
#endif

    switch (msg) {

    case WM_SIZE:
    // 最小化時はサイズが0になるため無視する
    if (wparam != SIZE_MINIMIZED) {
        int32_t width = LOWORD(lparam);
        int32_t height = HIWORD(lparam);
        WinApp* app = GetInstance();
        // 実際にサイズが変わったときだけフラグを立てる
        if (width != app->clientWidth_ || height != app->clientHeight_) {
            app->clientWidth_ = width;
            app->clientHeight_ = height;
            app->sizeChanged_ = true;
        }
    }
    break;

    case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

    case WM_SYSCOMMAND:
    // Altキー単押し（やF10）でウィンドウが「メニューモード」に入ると、次のキーが来るまでメッセージループが
    // 止まりゲームが固まる。このウィンドウにメニューは無いので、その要求だけ無視する
    // （スプライトエディタの Alt+クリック／Alt+ホイールを安心して使えるようにする）
    if ((wparam & 0xFFF0) == SC_KEYMENU) {
        return 0;
    }
    break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

WinApp* WinApp::GetInstance() {
    static WinApp instance;
    return &instance;
}

void WinApp::Initialize() {

    wc_.lpfnWndProc = WindowProc;
    wc_.lpszClassName = L"WindowClass";
    wc_.hInstance = GetModuleHandle(nullptr);
    wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc_);

    RECT wrc = {
        0,
        0,
        kClientWidth,
        kClientHeight
    };

    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    hwnd_ = CreateWindow(
        wc_.lpszClassName,
        title_.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        wc_.hInstance,
        nullptr
    );

    ShowWindow(hwnd_, SW_SHOW);
}

void WinApp::SetTitle(const std::wstring& title) {
    title_ = title;

    // ウィンドウ生成後ならその場でタイトルバーを書き換える
    if (hwnd_ != nullptr) {
        SetWindowTextW(hwnd_, title_.c_str());
    }
}

void WinApp::SetFullscreen(bool enable) {
    if (enable == fullscreen_) {
        return;
    }

    if (enable) {
        // 現在のスタイルとウィンドウ矩形を保存（復帰用）
        savedStyle_ = GetWindowLongPtrW(hwnd_, GWL_STYLE);
        savedExStyle_ = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
        GetWindowRect(hwnd_, &savedWindowRect_);

        // ウィンドウが乗っているモニタの矩形を取得
        HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = static_cast<DWORD>(sizeof(monitorInfo));
        GetMonitorInfoW(monitor, &monitorInfo);

        // 枠・タイトルバーを外し（WS_POPUP相当）、モニタ全体を覆う
        SetWindowLongPtrW(hwnd_, GWL_STYLE, savedStyle_ & ~static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW));
        SetWindowPos(
            hwnd_, HWND_TOP,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        fullscreen_ = true;
    } else {
        // 元のスタイル・位置へ戻す
        SetWindowLongPtrW(hwnd_, GWL_STYLE, savedStyle_);
        SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, savedExStyle_);
        SetWindowPos(
            hwnd_, HWND_NOTOPMOST,
            savedWindowRect_.left,
            savedWindowRect_.top,
            savedWindowRect_.right - savedWindowRect_.left,
            savedWindowRect_.bottom - savedWindowRect_.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        fullscreen_ = false;
    }
}

bool WinApp::ProcessMessage() {

    MSG msg{};

    // たまっているメッセージを今フレームで全部処理する。
    // 1フレームに1件だけだと、キー入力（1文字でKEYDOWN・CHAR・KEYUPの3件）やクリックが
    // 何フレームも遅れて届き、ImGuiの文字入力やボタン操作がもたつく
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT) {
            return false;
        }
    }

    return true;
}

} // namespace Engine
