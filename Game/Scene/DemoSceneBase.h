#pragma once

#include <cstdint>
#include <d3d12.h>
#include <vector>

#include "Engine/Camera/Camera.h"
#include "Engine/Culling/FrustumCulling.h"
#include "Engine/Light/DirectionalLight.h"
#include "Engine/Light/PointLight.h"
#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Rendering/Material.h"
#include "Game/Object/Skydome.h"
#include "Game/Scene/BaseScene.h"
// デバッグカメラはDebugビルド限定。このプロジェクトはReleaseでも_DEBUGが定義される
// （RuntimeLibrary=MultiThreadedDebug）ため、Release判定にはNDEBUGを使う。
#ifndef NDEBUG
#include "Engine/Camera/DebugCamera.h"
#endif

/// <summary>
/// 通常デモシーン共通の基底クラス。どのデモシーンでも使える「標準機能」
/// （カメラ・天球・平行光源・点光源・サウンド・デバッグカメラ・共通ImGui・立体視カメラ）を持ち、
/// Initialize/Update/Draw/DrawImGuiの共通の流れを提供する。
/// 派生シーンはOn～のフックをオーバーライドし、シーン固有のオブジェクトだけを扱う。
/// </summary>
class DemoSceneBase : public BaseScene {
public:
    // 共通リソースを生成し、最後に派生のOnInitializeを呼ぶ
    void Initialize() override;

    // サウンド入力→カメラ・デバッグカメラ→派生オブジェクト更新（OnUpdate）
    // →天球・光源・立体視カメラの順で毎フレーム更新する
    void Update() override;

    // 共通CBV・天球の描画後、標準PSOへ切り替えて派生のOnDrawを呼ぶ
    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) override;

#ifdef USE_IMGUI
    // 共通ウィンドウ（3D Objects/Camera/光源/Sound/Frustum Culling等）を構築し、
    // シーン固有部分は各On～ImGuiフックへ委譲する
    void DrawImGui() override;
#endif

protected:
    // --- 派生シーンが実装するフック ---

    // シーン固有のリソース生成（共通リソースの生成後に呼ばれる）
    virtual void OnInitialize(ID3D12Device* device) = 0;

    // シーン固有オブジェクトの更新（ワールド行列・定数バッファ書き込み・カリング判定）。
    // viewWidth/viewHeight: ゲーム描画先矩形の大きさ（スプライトの正射影などに使う）
    virtual void OnUpdate(const Engine::Frustum3D& frustum, float viewWidth, float viewHeight) = 0;

    // シーン固有オブジェクトの描画（標準PSO・共通CBV設定済みの状態で呼ばれる）
    virtual void OnDraw(ID3D12GraphicsCommandList* commandList) = 0;

#ifdef USE_IMGUI
    // "3D Objects"ウィンドウ内のシーン固有UI（天球ノードの前に呼ばれる）
    virtual void OnDrawObjectsImGui() = 0;

    // シーン固有の追加ウィンドウ（必要なシーンだけオーバーライドする）
    virtual void OnDrawExtraImGui() {}

    // "Frustum Culling"ウィンドウ内のシーン固有の判定結果表示
    virtual void OnDrawCullingImGui() = 0;

    // --- 派生シーンのImGui構築で使うヘルパー ---

    // カリング判定結果の表示用文字列
    static const char* VisibilityText(Engine::FrustumVisibility visibility);

    // マテリアルのライティング方式（なし/Lambert/Half Lambert）を選択するCombo
    static void DrawLightingModeCombo(Engine::Material& material);
#endif

#ifndef NDEBUG
    // デバッグカメラのピッキング対象（ワールド空間のバウンディング球）を追加する
    virtual void AppendPickTargets(std::vector<Engine::DebugCamera::PickTarget>& targets) const = 0;
#endif

    // --- 全デモシーン共通の「標準機能」（派生シーンから直接使える）---

    // シーン共通のテクスチャ（TextureManagerのハンドル。ImGuiのComboに対応）
    static constexpr int kTextureCount = 2;
    static const char* const kTextureItems[kTextureCount];  // Combo用の表示名
    uint32_t textureHandles_[kTextureCount] = {};           // 0:uvChecker / 1:monsterBall

    // 背景（最初に描画）
    Skydome skydome_;

    // カメラ
    // （立体視の視点別ビュー射影・視線追跡はBaseSceneのstereoCamera_が担当する）
    Engine::Camera camera_;

    // 平行光源（CPU側の値をImGuiで編集し、Updateで定数バッファへ書き込む）
    Engine::DirectionalLight light_{ { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, 1.0f, 1, {} };
    Engine::ConstantBuffer<Engine::DirectionalLight> lightCB_;

    // 点光源（最大kMaxPointLightCount個。ImGuiで個別編集し、Updateで定数バッファへ書き込む）
    Engine::PointLightGroup pointLights_;
    Engine::ConstantBuffer<Engine::PointLightGroup> pointLightCB_;

    // サウンド（Spaceキーまたは ImGuiのボタンで再生）
    size_t soundHandle_ = 0;     // Alarm01.wavのハンドル
    float soundVolume_ = 1.0f;   // ImGuiで調整する音量

#ifndef NDEBUG
    // デバッグカメラ（Debugビルドのみ。Releaseでは無効）
    Engine::DebugCamera debugCamera_;
#endif
};
