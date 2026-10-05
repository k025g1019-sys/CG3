#pragma once

#include "Engine/Core/Framework.h"
#include "Engine/Graphics/CameraCapture.h"
#include "Engine/Input/EyeTracker.h"
#include "Engine/Input/FaceTracker.h"
#include "Sandbox/Scene/SceneFactory.h"

/// <summary>
/// エンジン動作確認用アプリ（Sandbox）のアプリケーションクラス。
/// シーンの更新・描画はエンジン（SceneManager）が行い、ここでは
/// 視線追跡（アプリ内顔検出／共有メモリ）とWebカメラ表示を現在のシーンへ配線する。
/// Tキーで立体視デモシーンと通常デモシーン（直前にいた方）を、
/// Tabキーで通常デモシーン同士（kGame ⇔ kAxis）を切り替える。
/// </summary>
class SandboxApp : public Engine::Framework {
protected:

    void Initialize() override;

    void Finalize() override;

    void Update() override;

    // Webカメラ表示ON時、最新フレームをGPUテクスチャへ転送する
    void PreDraw(ID3D12GraphicsCommandList* commandList) override;

#ifdef USE_IMGUI
    void DrawImGui() override;
#endif

private:

    // 現在のシーンID（起動時のシーンはSceneFactory.hのkInitialSceneIdで指定する）
    SceneId sceneId_ = kInitialSceneId;

    // 立体視デモへ入る直前にいた通常デモシーン（Tキーで立体視デモから戻る先）
    SceneId lastDemoSceneId_ = SceneId::kGame;

    // --- 視線追跡とWebカメラ ---
    Engine::CameraCapture camera_;    // Webカメラ取得・表示（使わない間はワーカー停止）
    Engine::FaceTracker faceTracker_; // アプリ内顔検出（Webカメラのフレームから頭位置を推定）
    Engine::EyeTracker eyeTracker_;   // 外部プロセスから共有メモリ経由で視線を受信（差し替え用に残置）

    bool useEyeTracking_ = false;  // 視線追跡でゲーム内カメラを連動させる（ImGuiでON/OFF）
    bool showCamera_ = false;      // 実カメラの映像をWebcamウィンドウに映す（ImGuiでON/OFF）
    int gazeSource_ = 0;           // 視線の取得元（0=アプリ内顔検出 / 1=共有メモリ）
};
