#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Object3D.h"
#include "Engine/Scene/BaseScene.h"
#include "Sandbox/Object/AxisGizmo.h"
#include "Sandbox/Object/PadObjectController.h"
#include "Sandbox/Object/Skydome.h"
// デバッグカメラはRelease以外。このプロジェクトはReleaseでも_DEBUGが定義される
// （RuntimeLibrary=MultiThreadedDebug）ため、Release判定にはNDEBUGを使う。
#ifndef NDEBUG
#include "Engine/Camera/DebugCamera.h"
#endif

/// <summary>
/// 立体視デモシーン。テクスチャ付きの立方体を奥行き違いに並べ、飛び出し・引っ込みを確認する。
/// 立方体はImGuiで追加・削除でき、Transform（中心基準）・色も個別に編集できる。
/// カメラ・光源・立体視の視点別ビュー射影などの定型処理はエンジンのBaseSceneが行う。
/// </summary>
class StereoDemoScene : public Engine::BaseScene {
protected:
    // 立方体・天球・ギズモの生成とカメラ・光源の初期設定
    void OnInitialize() override;

    // デバッグカメラ・パッド操作・各立方体・天球の更新
    void OnUpdate() override;

    // 天球 → 立方体 → 軸ギズモ の順で描画する
    void OnDraw() override;

#ifdef USE_IMGUI
    // 開発用ImGuiウィンドウの構築
    void OnDrawImGui() override;
#endif

#ifndef NDEBUG
    // デバッグカメラが有効なときは、そのビュー行列で描画する
    Engine::Matrix4x4 CalcViewMatrix() const override;
#endif

private:
    // 立方体を1個追加する（position:中心位置 / rotateY:Y軸回転）
    void AddCube(const Engine::Vector3& position, float rotateY);

    // --- メッシュ（全立方体で共有する立方体形状）---
    Engine::Mesh cubeMesh_;

    // --- 立方体（可変長。追加・削除でvectorが再確保されても各Object3Dが動かないようunique_ptrで保持）---
    std::vector<std::unique_ptr<Engine::Object3D>> cubes_;

    // --- テクスチャ（resources/tilemap_ground_01_1tileonly.png）---
    uint32_t cubeTextureHandle_ = 0;

    // --- 背景（最初に描画）---
    Skydome skydome_;

    // --- パッドによる選択立方体の操作（対象リストはUpdateで毎フレーム組み立てる）---
    PadObjectController padController_;

    // --- 選択立方体のローカル回転軸ギズモ（X=赤/Y=緑/Z=青）---
    AxisGizmo axisGizmo_;

#ifndef NDEBUG
    // --- デバッグカメラ（Release以外）---
    Engine::DebugCamera debugCamera_;
#endif
};
