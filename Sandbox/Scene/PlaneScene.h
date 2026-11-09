#pragma once

#include <cstdint>

#include "Engine/Rendering/InstancedObject3D.h"
#include "Sandbox/Scene/DemoSceneBase.h"

// 最初のデモシーン。Instancingで板ポリ（plane.obj）10枚を、1回の描画命令でまとめて描画する
// （パーティクルの土台として作ったシーン。各板ポリは少しずつずらして並べる）。
// Tabキーで 板ポリ → パーティクル（GameScene）→ 鏡面反射（AxisScene）→ 板ポリ の順に切り替わる。
// カメラ・天球・光源・サウンド・デバッグカメラ等の標準機能はDemoSceneBase（とエンジンのBaseScene）が提供する。
class PlaneScene : public DemoSceneBase {
protected:
    // 板ポリ（インスタンス）の生成・配置
    void OnInitializeObjects() override;

    // 板ポリの行列計算・StructuredBuffer更新
    void OnUpdateObjects() override;

    // 板ポリの描画（DrawInstanced 1回で全インスタンスを描く）
    void OnDrawObjects() override;

#ifdef USE_IMGUI
    // "3D Objects"ウィンドウ内の板ポリのUI（描画するインスタンス数・共通の色・インスタンスごとのTransform）
    void OnDrawObjectsImGui() override;

    // カリング判定結果表示（Instancingではインスタンスごとのカリングをしない）
    void OnDrawCullingImGui() override;
#endif

#ifndef NDEBUG
    // デバッグカメラのピッキング対象（板ポリはObject3Dではないため無し）
    void AppendPickTargets(std::vector<Engine::DebugCamera::PickTarget>& targets) const override;
#endif

    // パッドで操作できるオブジェクト（板ポリはObject3Dではないため無し）
    void AppendPadTargets(std::vector<PadObjectController::Target>& targets) override;

private:
    // 板ポリの数
    static constexpr uint32_t kNumInstance = 10;

    // 板ポリ（plane.obj）をInstancingでまとめて描くオブジェクト
    Engine::InstancedObject3D planes_;
};
