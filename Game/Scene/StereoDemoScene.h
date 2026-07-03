#pragma once

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <vector>

#include "Engine/Camera/Camera.h"
#include "Engine/Light/DirectionalLight.h"
#include "Engine/Light/PointLight.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Object/Skydome.h"
#include "Game/Scene/BaseScene.h"
// デバッグカメラはDebugビルド限定。このプロジェクトはReleaseでも_DEBUGが定義される
// （RuntimeLibrary=MultiThreadedDebug）ため、Release判定にはNDEBUGを使う。
#ifndef NDEBUG
#include "Engine/Camera/DebugCamera.h"
#endif

/// <summary>
/// 立体視デモシーン。テクスチャ付きの立方体を奥行き違いに並べ、飛び出し・引っ込みを確認する。
/// 立方体はImGuiで追加・削除でき、Transform（中心基準）・色も個別に編集できる。
/// </summary>
class StereoDemoScene : public BaseScene {
public:
    // 各リソースを生成する（DirectXCore・PipelineManager・TextureManagerの初期化後に呼ぶ）
    void Initialize() override;

    // UI操作を反映した行列計算・定数バッファ更新・カリング判定。
    // 視点ごとのビュー射影（平行配置＋オフアクシス射影）もここで更新する。
    void Update() override;

    // 描画コマンドを積む。
    // viewIndex:描画する視点（この視点のビュー射影CBufferをVS[b1]へバインドする）。
    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) override;

#ifdef USE_IMGUI
    // 開発用ImGuiウィンドウの構築
    void DrawImGui() override;
#endif

private:
    // 立方体を1個追加する（position:中心位置 / rotateY:Y軸回転）
    void AddCube(const Vector3& position, float rotateY);

    // --- メッシュ（全立方体で共有する立方体形状）---
    Mesh cubeMesh_;

    // --- 立方体（可変長。追加・削除でvectorが再確保されても各Object3Dが動かないようunique_ptrで保持）---
    std::vector<std::unique_ptr<Object3D>> cubes_;

    // --- テクスチャ（resources/tilemap_ground_01_1tileonly.png）---
    uint32_t cubeTextureHandle_ = 0;

    // --- 背景（最初に描画）---
    Skydome skydome_;

    // --- カメラ ---
    Camera camera_;

    // --- 平行光源（CPU側の値をImGuiで編集し、Updateで定数バッファへ書き込む）---
    // 立方体の面の向きが分かるよう、初期方向は斜め下向きにする。
    DirectionalLight light_{ { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.4f, -1.0f, 0.6f }, 1.0f, 1, {} };
    ConstantBuffer<DirectionalLight> lightCB_;

    // --- 点光源（このシーンでは未使用。共通ルートシグネチャ（PS b2）が要求するため全灯無効で置く）---
    PointLightGroup pointLights_;
    ConstantBuffer<PointLightGroup> pointLightCB_;

#ifndef NDEBUG
    // --- デバッグカメラ（Debugビルドのみ。Releaseでは無効）---
    DebugCamera debugCamera_;
#endif
};
