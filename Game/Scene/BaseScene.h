#pragma once

#include <cstdint>
#include <d3d12.h>

#include "Engine/Camera/StereoCamera.h"

/// <summary>
/// シーンの基底クラス。MyGameがこのインターフェース越しに現在のシーンを駆動する。
/// 全シーン共通の立体視カメラ（視点別ビュー射影＋頭連動）とアスペクト補正を持つ。
/// </summary>
class BaseScene {
public:

    virtual ~BaseScene() = default;

    // 各リソースを生成する（エンジン初期化後、またはシーン切り替え時に呼ばれる）
    virtual void Initialize() = 0;

    // UI操作を反映した行列計算・定数バッファ更新・カリング判定（毎フレーム呼ぶ）
    virtual void Update() = 0;

    // 描画コマンドを積む。
    // viewIndex:描画する視点（この視点のビュー射影CBufferをVS[b1]へバインドする）。
    // 立体視有効時はStereoRendererの視点数ぶん呼ばれ、無効時はviewIndex=0で1回呼ばれる。
    virtual void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) = 0;

#ifdef USE_IMGUI
    // 開発用ImGuiウィンドウの構築
    virtual void DrawImGui() = 0;
#endif

    // 視線追跡の状態を反映する（Updateの前に毎フレーム呼ぶ）。
    // enabled:視線追跡ON / gazeX,gazeY:正規化視線[-1..1]（-1左下 +1右上） / headZ:奥行き[-1..1]
    // enabledのとき、頭連動オフアクシス投影でカメラ視点をずらす（立体視OFFでも運動視差として効く）。
    void SetEyeTracking(bool enabled, float gazeX, float gazeY, float headZ) {
        stereoCamera_.SetEyeTracking(enabled, gazeX, gazeY, headZ);
    }

    // 描画先のアスペクト補正（Updateの前に毎フレーム呼ぶ）。
    // ゲーム描画が表示される横方向の割合を渡す（通常=1.0 / 左右分割でゲームが左半分のとき=0.5）。
    // 投影のアスペクト比をこの割合ぶん横に詰め、分割しても物体が伸び縮みしないようにする。
    void SetRenderAspectScale(float horizontalScale) { renderAspectScale_ = horizontalScale; }

protected:

    // 立体視＋頭連動の視点別ビュー射影（各シーンのInitializeで生成し、Updateの最後に更新する）
    StereoCamera stereoCamera_;

    // 描画先の横方向の割合（1.0=画面全体 / 0.5=左右分割でゲームが左半分）。投影アスペクト補正に使う。
    float renderAspectScale_ = 1.0f;
};
