#pragma once

#include <cstdint>

#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Object3D.h"
#include "Sandbox/Scene/DemoSceneBase.h"

// 2つ目のデモシーン。三角形・球・axis.obj・teapot.obj・multiMesh.objを描画する
// （Tabキーで最初のシーンと切り替え）。
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBase（とエンジンのBaseScene）が提供する。
class AxisScene : public DemoSceneBase {
protected:
    // 三角形・OBJ・球の生成
    void OnInitializeObjects() override;

    // 各オブジェクトの行列計算・定数バッファ更新
    void OnUpdateObjects() override;

    // 各オブジェクトの描画コマンドを積む
    void OnDrawObjects() override;

#ifdef USE_IMGUI
    // "3D Objects"ウィンドウ内の各オブジェクトの編集UI
    void OnDrawObjectsImGui() override;

    // 各オブジェクトのカリング判定結果表示
    void OnDrawCullingImGui() override;
#endif

#ifndef NDEBUG
    // デバッグカメラのピッキング対象（各オブジェクト）
    void AppendPickTargets(std::vector<Engine::DebugCamera::PickTarget>& targets) const override;
#endif

    // パッドで操作できるオブジェクト（各オブジェクト）
    void AppendPadTargets(std::vector<PadObjectController::Target>& targets) override;

private:
    // --- 自前で頂点を作るメッシュ（OBJのメッシュはMeshManagerが読み込み・共有する）---
    Engine::Mesh triangleMesh_;   // 三角形2枚（6頂点。2枚目は1枚目を貫通する）
    Engine::Mesh sphereMesh_;     // 球（分割数をImGuiで変えられるよう自前で持つ）

    // --- 描画オブジェクト ---
    Engine::Object3D triangle_;
    Engine::Object3D axis_;       // axis.obj
    Engine::Object3D teapot_;     // teapot.obj（ユタ・ティーポット）
    Engine::Object3D multiMesh_;  // multiMesh.obj（2サブメッシュ・1マテリアル）
    Engine::Object3D sphere_;

    // --- 球の分割数（ImGuiで変更すると頂点を再生成する）---
    uint32_t subdivision_ = 16;
    uint32_t prevSubdivision_ = 16;

    // --- テクスチャ選択（ImGuiのComboに対応）---
    // OBJモデルは0="MTL (default)"（mtl由来）、1以降でtextureHandles_の一括上書き
    int axisTextureIndex_ = 0;
    int teapotTextureIndex_ = 0;
    int multiMeshTextureIndex_ = 0;
    // 三角形・球はtextureHandles_のインデックス（球の初期テクスチャはuvChecker）
    int triangleTextureIndex_ = 0;
    int sphereTextureIndex_ = 0;
};
