#pragma once

#include <cstdint>

#include "Engine/Rendering/ParticleSystem.h"
#include "Sandbox/Scene/DemoSceneBase.h"

// 最初のデモシーン。パーティクルの仕組みの土台として、Instancingで板ポリ（plane.obj）10個を
// 1回の描画命令でまとめて描画する（それまで置いていたOBJモデル・スプライトはAxisSceneへ移した）。
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBase（とエンジンのBaseScene）が提供する。
class GameScene : public DemoSceneBase {
protected:
    // パーティクル（板ポリのインスタンス）の生成・配置
    void OnInitializeObjects() override;

    // パーティクルの行列計算・StructuredBuffer更新
    void OnUpdateObjects() override;

    // パーティクルの描画（DrawInstanced 1回で全インスタンスを描く）
    void OnDrawObjects() override;

#ifdef USE_IMGUI
    // "3D Objects"ウィンドウ内のパーティクルの編集UI（インスタンス数・色・各インスタンスのTransform）
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
    // インスタンス数（描画できるパーティクルの最大数）
    static constexpr uint32_t kNumInstance = 10;

    // 板ポリ（plane.obj）をInstancingでまとめて描くパーティクル
    Engine::ParticleSystem particles_;
};
