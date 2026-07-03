#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>

#include "Engine/Camera/Camera.h"
#include "Engine/Light/DirectionalLight.h"
#include "Engine/Light/PointLight.h"
#include "Game/Object/Skydome.h"
#include "Game/Scene/BaseScene.h"
#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Object3D.h"
#include "Engine/Rendering/Sprite.h"
// デバッグカメラはDebugビルド限定。このプロジェクトはReleaseでも_DEBUGが定義される
// （RuntimeLibrary=MultiThreadedDebug）ため、Release判定にはNDEBUGを使う。
#ifndef NDEBUG
#include "Engine/Camera/DebugCamera.h"
#endif

// デモ用シーン。三角形・球・OBJ・スプライト・天球と開発用ImGuiを保持する。
class GameScene : public BaseScene {
public:
    // 各リソースを生成する（DirectXCore・PipelineManager・TextureManagerの初期化後に呼ぶ）
    void Initialize() override;

    // UI操作を反映した行列計算・定数バッファ更新・カリング判定。
    // 立体視が有効なときは視点ごとのビュー射影（平行配置＋オフアクシス射影）もここで更新する。
    void Update() override;

#ifdef USE_IMGUI
    // 開発用ImGuiウィンドウの構築
    void DrawImGui() override;
#endif

    // 描画コマンドを積む。
    // viewIndex:描画する視点（この視点のビュー射影CBufferをVS[b1]へバインドする）。
    // 立体視有効時はStereoRendererの視点数ぶん呼ばれ、無効時はviewIndex=0で1回呼ばれる。
    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) override;

private:
    // --- メッシュ（形状データ）---
    Mesh triangleMesh_;
    Mesh sphereMesh_;
    Mesh objMesh_;

    // --- 描画オブジェクト ---
    Object3D triangle_;
    Object3D sphere_;
    Object3D obj_;
    Sprite sprite_;
    Skydome skydome_;  // 背景（最初に描画）

    // --- カメラ ---
    // （立体視の視点別ビュー射影・視線追跡はBaseSceneのstereoCamera_が担当する）
    Camera camera_;

    // --- 平行光源（CPU側の値をImGuiで編集し、Updateで定数バッファへ書き込む）---
    DirectionalLight light_{ { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, 1.0f, 1, {} };
    ConstantBuffer<DirectionalLight> lightCB_;

    // --- 点光源（最大kMaxPointLightCount個。ImGuiで個別編集し、Updateで定数バッファへ書き込む）---
    PointLightGroup pointLights_;
    ConstantBuffer<PointLightGroup> pointLightCB_;

    // --- 球の分割数（ImGuiで変更すると頂点を再生成する）---
    uint32_t subdivision_ = 16;
    uint32_t prevSubdivision_ = 16;

    // --- シーンで使うテクスチャ（TextureManagerのハンドル。ImGuiのComboに対応）---
    uint32_t textureHandles_[2] = {};  // 0:uvChecker / 1:monsterBall
    int triangleTextureIndex_ = 0;
    int sphereTextureIndex_ = 1;
    int objTextureIndex_ = 0;
    int spriteTextureIndex_ = 0;

    // --- サウンド ---
    size_t soundHandle_ = 0;     // Alarm01.wavのハンドル
    float soundVolume_ = 1.0f;   // ImGuiで調整する音量

    // --- スプライト描画のオン/オフ（ImGuiで切り替え） ---
    bool drawSprite_ = false;

#ifndef NDEBUG
    // --- デバッグカメラ（Debugビルドのみ。Releaseでは無効）---
    DebugCamera debugCamera_;
#endif
};
