#pragma once

#include <cstdint>
#include <d3d12.h>

#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Object3D.h"
#include "Game/Scene/DemoSceneBase.h"

// 2つ目のデモシーン。axis.objと球を描画する（Tabキーで最初のシーンと切り替え）。
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBaseが提供する。
class AxisScene : public DemoSceneBase {
protected:
    // OBJ・球の生成
    void OnInitialize(ID3D12Device* device) override;

    // 各オブジェクトの行列計算・定数バッファ更新・カリング判定
    void OnUpdate(const Engine::Frustum3D& frustum, float viewWidth, float viewHeight) override;

    // 各オブジェクトの描画コマンドを積む
    void OnDraw(ID3D12GraphicsCommandList* commandList) override;

#ifdef USE_IMGUI
    // "3D Objects"ウィンドウ内のOBJ・球の編集UI
    void OnDrawObjectsImGui() override;

    // 各オブジェクトのカリング判定結果表示
    void OnDrawCullingImGui() override;
#endif

#ifndef NDEBUG
    // デバッグカメラのピッキング対象（OBJ・球）
    void AppendPickTargets(std::vector<Engine::DebugCamera::PickTarget>& targets) const override;
#endif

private:
    // --- メッシュ（形状データ）---
    Engine::Mesh objMesh_;     // axis.obj
    Engine::Mesh sphereMesh_;

    // --- 描画オブジェクト ---
    Engine::Object3D obj_;
    Engine::Object3D sphere_;

    // --- 球の分割数（ImGuiで変更すると頂点を再生成する）---
    uint32_t subdivision_ = 16;
    uint32_t prevSubdivision_ = 16;

    // --- テクスチャ選択（DemoSceneBaseのtextureHandles_のインデックス。ImGuiのComboに対応）---
    int objTextureIndex_ = 0;
    int sphereTextureIndex_ = 1;
};
