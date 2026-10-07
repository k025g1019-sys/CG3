#pragma once

#include "Engine/Rendering/Object3D.h"
#include "Engine/Rendering/Sprite.h"
#include "Sandbox/Scene/DemoSceneBase.h"

// 最初のデモシーン。2Dスプライトと5つのOBJモデル
// （plane・bunny・multiMaterial・suzanne・fence）を描画する。
// Debug・Developmentではスプライトエディタで描いた内容をそのまま映すプレビュー用スプライトも置く。
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBase（とエンジンのBaseScene）が提供する。
class GameScene : public DemoSceneBase {
protected:
    // OBJモデル・スプライトの生成
    void OnInitializeObjects() override;

    // 各オブジェクトの行列計算・定数バッファ更新
    void OnUpdateObjects() override;

    // 各オブジェクトの描画コマンドを積む
    void OnDrawObjects() override;

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
    // --- 描画オブジェクト（OBJのメッシュはMeshManagerが読み込み・共有する）---
    Engine::Object3D plane_;          // plane.obj
    Engine::Object3D bunny_;          // bunny.obj（スタンフォードバニー）
    Engine::Object3D multiMaterial_;  // multiMaterial.obj（2サブメッシュ・2マテリアル）
    Engine::Object3D suzanne_;        // suzanne.obj（UVなし・テクスチャなし）
    Engine::Object3D fence_;          // fence.obj（透明部分をdiscardで抜く）
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

#ifdef USE_IMGUI
    // --- スプライトエディタのプレビュー（キャンバスのテクスチャをそのまま表示。サイズはキャンバスに追従）---
    Engine::Sprite editorSprite_;
    bool drawEditorSprite_ = true;
#endif
};
