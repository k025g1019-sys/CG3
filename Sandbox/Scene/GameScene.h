#pragma once

#include <cstdint>

#include "Engine/Rendering/ParticleSystem.h"
#include "Sandbox/Scene/DemoSceneBase.h"

// 最初のデモシーン。板ポリ（plane.obj）のパーティクル10個を、Instancingで1回の描画命令でまとめて描画する。
// パーティクルは乱数で発生し、1～3秒かけて薄くなって消え、すぐ再発生する（加算合成で描く）。
// （それまで置いていたOBJモデル・スプライトはAxisSceneへ移した）
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBase（とエンジンのBaseScene）が提供する。
class GameScene : public DemoSceneBase {
protected:
    // パーティクルの生成
    void OnInitializeObjects() override;

    // パーティクルの移動・消滅と再発生・StructuredBuffer更新
    void OnUpdateObjects() override;

    // パーティクルの描画（DrawInstanced 1回で全パーティクルを描く）
    void OnDrawObjects() override;

#ifdef USE_IMGUI
    // "3D Objects"ウィンドウ内のパーティクルのUI（パーティクルの数の表示・共通の色）
    void OnDrawObjectsImGui() override;

    // カリング判定結果表示（パーティクルはインスタンスごとのカリングをしない）
    void OnDrawCullingImGui() override;
#endif

#ifndef NDEBUG
    // デバッグカメラのピッキング対象（パーティクルはObject3Dではないため無し）
    void AppendPickTargets(std::vector<Engine::DebugCamera::PickTarget>& targets) const override;
#endif

    // パッドで操作できるオブジェクト（パーティクルはObject3Dではないため無し）
    void AppendPadTargets(std::vector<PadObjectController::Target>& targets) override;

private:
    // インスタンス数（パーティクルの数）
    static constexpr uint32_t kNumInstance = 10;

    // 板ポリ（plane.obj）をInstancingでまとめて描くパーティクル
    Engine::ParticleSystem particles_;
};
