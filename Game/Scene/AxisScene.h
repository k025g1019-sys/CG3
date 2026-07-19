#pragma once

#include <cstdint>
#include <d3d12.h>

#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Scene/DemoSceneBase.h"

// 2つ目のデモシーン。三角形・球・axis.obj・teapot.obj・multiMesh.objを描画する
// （Tabキーで最初のシーンと切り替え）。
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBaseが提供する。
class AxisScene : public DemoSceneBase {
protected:
    // 三角形・OBJ・球の生成
    void OnInitialize(ID3D12Device* device) override;

    // 各オブジェクトの行列計算・定数バッファ更新・カリング判定
    void OnUpdate(const Engine::Frustum3D& frustum, float viewWidth, float viewHeight) override;

    // 各オブジェクトの描画コマンドを積む
    void OnDraw(ID3D12GraphicsCommandList* commandList) override;

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

private:
    // --- メッシュ（形状データ）---
    Engine::Mesh triangleMesh_;   // 三角形2枚（6頂点。2枚目は1枚目を貫通する）
    Engine::Mesh axisMesh_;       // axis.obj
    Engine::Mesh teapotMesh_;     // teapot.obj（ユタ・ティーポット）
    Engine::Mesh multiMeshMesh_;  // multiMesh.obj（2サブメッシュ・1マテリアル）
    Engine::Mesh sphereMesh_;

    // --- 描画オブジェクト ---
    Engine::Object3D triangle_;
    Engine::Object3D axis_;
    Engine::Object3D teapot_;
    Engine::Object3D multiMesh_;
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
