#pragma once

#include <memory>

#include "Engine/Core/Framework.h"
#include "Engine/Graphics/CameraCapture.h"
#include "Engine/Input/EyeTracker.h"
#include "Engine/Input/FaceTracker.h"
#include "Game/Scene/SceneFactory.h"

/// <summary>
/// このゲームのアプリケーションクラス。
/// Frameworkのメインループから呼ばれる更新・描画を現在のシーンへ委譲し、
/// 視線追跡（アプリ内顔検出／共有メモリ）とWebカメラ表示をシーンへ配線する。
/// Tキーで立体視デモシーンと従来デモシーンを切り替える。
/// </summary>
class MyGame : public Framework {
protected:

    void Initialize() override;

    void Finalize() override;

    void Update() override;

    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) override;

    // Webカメラ表示ON時、最新フレームをGPUテクスチャへ転送する
    void PreDraw(ID3D12GraphicsCommandList* commandList) override;

#ifdef USE_IMGUI
    void DrawImGui() override;
#endif

private:

    // 現在のシーン（Tキーで切り替え。Finalizeでエンジン終了処理より先に解放する）
    std::unique_ptr<BaseScene> scene_;

    // 現在のシーンID（起動時のシーンはSceneFactory.hのkInitialSceneIdで指定する）
    SceneId sceneId_ = kInitialSceneId;

    // --- 視線追跡とWebカメラ ---
    CameraCapture camera_;    // Webカメラ取得・表示（使わない間はワーカー停止）
    FaceTracker faceTracker_; // アプリ内顔検出（Webカメラのフレームから頭位置を推定）
    EyeTracker eyeTracker_;   // 外部プロセスから共有メモリ経由で視線を受信（差し替え用に残置）

    bool useEyeTracking_ = false;  // 視線追跡でゲーム内カメラを連動させる（ImGuiでON/OFF）
    bool showCamera_ = false;      // 実カメラの映像をWebcamウィンドウに映す（ImGuiでON/OFF）
    int gazeSource_ = 0;           // 視線の取得元（0=アプリ内顔検出 / 1=共有メモリ）
};
