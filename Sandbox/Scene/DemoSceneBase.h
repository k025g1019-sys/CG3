#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Culling/FrustumCulling.h"
#include "Engine/Math/Curve.h"
#include "Engine/Rendering/Material.h"
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
/// 通常デモシーン共通の基底クラス。エンジンの BaseScene（カメラ・光源・描画設定）の上に、
/// デモ共通の機能（天球・サウンド・パッド操作・軸ギズモ・デバッグカメラ・共通ImGui）を載せる。
/// 派生シーンは On～Objects などのフックをオーバーライドし、シーン固有のオブジェクトだけを扱う。
/// </summary>
class DemoSceneBase : public Engine::BaseScene {
protected:
    // --- BaseScene のフック（デモ共通の処理を行い、下の On～Objects を呼ぶ）---

    // 共通リソースを生成し、最後に OnInitializeObjects を呼ぶ
    void OnInitialize() override;

    // サウンド入力 → パッド操作 → OnUpdateObjects → デバッグカメラ → 天球 の順で更新する
    void OnUpdate() override;

    // 天球 → OnDrawObjects → 軸ギズモ の順で描画する
    void OnDraw() override;

#ifdef USE_IMGUI
    // 共通ウィンドウ（3D Objects/Camera/光源/Sound/Frustum Culling等）を構築し、
    // シーン固有部分は各On～ImGuiフックへ委譲する
    void OnDrawImGui() override;
#endif

#ifndef NDEBUG
    // デバッグカメラが有効なときは、そのビュー行列で描画する
    Engine::Matrix4x4 CalcViewMatrix() const override;
#endif

    // --- 派生シーンが実装するフック ---

    // シーン固有のリソース生成（共通リソースの生成後に呼ばれる）
    virtual void OnInitializeObjects() = 0;

    // シーン固有オブジェクトの更新（各オブジェクトの Update）
    virtual void OnUpdateObjects() = 0;

    // シーン固有オブジェクトの描画（各オブジェクトの Draw。標準PSO・共通CBV設定済みの状態で呼ばれる）
    virtual void OnDrawObjects() = 0;

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

    // マテリアル編集UI（色・ライティング・UVTransform）。マテリアルが1個ならそのまま並べ、
    // 複数（マルチマテリアルOBJ）ならmtlのマテリアル名のTreeNodeに分けて個別に編集する
    static void DrawMaterialEditor(Engine::Object3D& object);

    // OBJモデル用のテクスチャCombo。0:"MTL (default)"＝サブメッシュごとのmtl由来テクスチャ、
    // 1以降:共有テクスチャ（textureHandles_）で全サブメッシュを一括上書き
    void DrawModelTextureCombo(Engine::Object3D& object, int& textureIndex);

    // サブメッシュ構成（名前・マテリアル・頂点数・テクスチャ）の読み取り専用表示
    static void DrawSubMeshInfo(const Engine::Mesh& mesh);
#endif

#ifndef NDEBUG
    // デバッグカメラのピッキング対象（ワールド空間のバウンディング球）を追加する
    virtual void AppendPickTargets(std::vector<Engine::DebugCamera::PickTarget>& targets) const = 0;
#endif

    // パッド（XBoxコントローラー）で操作できるオブジェクトを追加する
    virtual void AppendPadTargets(std::vector<PadObjectController::Target>& targets) = 0;

    // デバッグ線描画・当たり判定・曲線の確認（ImGuiの"Debug Draw"でON/OFF）
    void DrawDebugShapes();

    // --- 全デモシーン共通の「標準機能」（派生シーンから直接使える）---
    // （カメラ camera_・平行光源 directionalLight_・点光源 pointLights_ は BaseScene が持つ）

    // シーン共通のテクスチャ（TextureManagerのハンドル。ImGuiのComboに対応）
    static constexpr int kTextureCount = 2;
    static const char* const kTextureItems[kTextureCount];  // Combo用の表示名
    uint32_t textureHandles_[kTextureCount] = {};           // 0:uvChecker / 1:monsterBall

    // 背景（最初に描画）
    Skydome skydome_;

    // サウンド（Spaceキーまたは ImGuiのボタンで再生）
    size_t soundHandle_ = 0;     // Alarm01.wavのハンドル
    float soundVolume_ = 1.0f;   // ImGuiで調整する音量

    // パッドによる選択オブジェクト操作（対象は派生シーンがAppendPadTargetsで追加する）
    PadObjectController padController_;

    // 選択オブジェクトのローカル回転軸ギズモ（X=赤/Y=緑/Z=青）
    AxisGizmo axisGizmo_;

    // --- デバッグ線描画のデモ（ImGuiの"Debug Draw"で切り替え）---
    bool showColliders_ = false;  // 各オブジェクトのバウンディング球（他と重なっていれば赤、それ以外は緑）
    bool showGrid_ = false;       // XZ平面のグリッド
    bool showCurve_ = false;      // ベジェ曲線と、その上を往復するマーカー
    Engine::Curve curve_{ { -3.0f, 0.0f, 3.0f }, { 0.0f, 4.0f, 3.0f }, { 3.0f, 0.0f, 3.0f } };
    float curveMarkerSpeed_ = 1.0f;  // マーカーの往復の速さ

#ifndef NDEBUG
    // デバッグカメラ（Release以外。Enterで有効・無効を切り替える）
    Engine::DebugCamera debugCamera_;
#endif
};
