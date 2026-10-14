#pragma once

#include <cstdint>

#include "Engine/Rendering/ParticleEmitter.h"
#include "Engine/Rendering/ParticleSystem.h"
#include "Sandbox/Scene/DemoSceneBase.h"

// 最初のデモシーン。板ポリ（plane.obj）のパーティクルを、Instancingで1回の描画命令でまとめて描画する。
// パーティクルはエミッタから0.5秒ごとに3個ずつ発生し（最大100個）、1～3秒かけて薄くなって消える
// （加算合成で描き、ビルボードで常にカメラを向く）。
// （それまで置いていたOBJモデル・スプライトはAxisSceneへ移した）
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBase（とエンジンのBaseScene）が提供する。
class GameScene : public DemoSceneBase {
protected:
    // パーティクルの初期化とエミッタの設定
    void OnInitializeObjects() override;

    // エミッタからの発生・パーティクルの移動と消滅・StructuredBuffer更新・エミッタの範囲表示
    void OnUpdateObjects() override;

    // パーティクルの描画（DrawInstanced 1回で全パーティクルを描く）
    void OnDrawObjects() override;

#ifdef USE_IMGUI
    // "3D Objects"ウィンドウ内のパーティクルのUI
    // （パーティクルの数の表示・ビルボードの切り替え・共通の色・エミッタの設定・発生と削除のボタン・範囲表示の切り替え）
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
    // インスタンスの最大数（同時に出せるパーティクルの最大数）
    static constexpr uint32_t kNumMaxInstance = 100;

    // 板ポリ（plane.obj）をInstancingでまとめて描くパーティクル
    Engine::ParticleSystem particles_;

    // パーティクルの発生源（発生位置・範囲・1回あたりの発生数・発生頻度など）
    Engine::ParticleEmitter emitter_;

    // エミッタの発生範囲を線で表示するか
    bool showEmitterRange_ = true;
};
