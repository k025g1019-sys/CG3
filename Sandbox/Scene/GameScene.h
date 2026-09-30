#pragma once

#include <d3d12.h>

#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Object3D.h"
#include "Engine/Rendering/Sprite.h"
#include "Sandbox/Scene/DemoSceneBase.h"

// 最初のデモシーン。2Dスプライトと4つのOBJモデル
// （plane・bunny・multiMaterial・suzanne）を描画する。
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBaseが提供する。
class GameScene : public DemoSceneBase {
protected:
    // OBJモデル・スプライトの生成
    void OnInitialize(ID3D12Device* device) override;

    // 各オブジェクトの行列計算・定数バッファ更新・カリング判定
    void OnUpdate(const Engine::Frustum3D& frustum, float viewWidth, float viewHeight) override;

    // 各オブジェクトの描画コマンドを積む
    void OnDraw(ID3D12GraphicsCommandList* commandList) override;

#ifdef USE_IMGUI
    // "3D Objects"ウィンドウ内の各OBJモデルの編集UI
    void OnDrawObjectsImGui() override;

    // "2D Objects"ウィンドウ（スプライトの編集UI）
    void OnDrawExtraImGui() override;

    // 各オブジェクトのカリング判定結果表示
    void OnDrawCullingImGui() override;
#endif

#ifndef NDEBUG
    // デバッグカメラのピッキング対象（各OBJモデル）
    void AppendPickTargets(std::vector<Engine::DebugCamera::PickTarget>& targets) const override;
#endif

    // パッドで操作できるオブジェクト（各OBJモデル）
    void AppendPadTargets(std::vector<PadObjectController::Target>& targets) override;

private:
    // --- メッシュ（形状データ）---
    Engine::Mesh planeMesh_;          // plane.obj
    Engine::Mesh bunnyMesh_;          // bunny.obj（スタンフォードバニー）
    Engine::Mesh multiMaterialMesh_;  // multiMaterial.obj（2サブメッシュ・2マテリアル）
    Engine::Mesh suzanneMesh_;        // suzanne.obj（UVなし・テクスチャなし）
    Engine::Mesh fenceMesh_;

    // --- 描画オブジェクト ---
    Engine::Object3D plane_;
    Engine::Object3D bunny_;
    Engine::Object3D multiMaterial_;
    Engine::Object3D suzanne_;
    Engine::Object3D fence_;
    Engine::Sprite sprite_;

    // --- テクスチャ選択（ImGuiのComboに対応）---
    // OBJモデルは0="MTL (default)"（mtl由来）、1以降でtextureHandles_の一括上書き
    int planeTextureIndex_ = 0;
    int bunnyTextureIndex_ = 0;
    int multiMaterialTextureIndex_ = 0;
    int suzanneTextureIndex_ = 0;
    int fenceTextureIndex_ = 0;
    // スプライトはtextureHandles_のインデックス
    int spriteTextureIndex_ = 0;

    // --- スプライト描画のオン/オフ（ImGuiで切り替え） ---
    bool drawSprite_ = true;
};
