#pragma once

#include <cstdint>
#include <d3d12.h>

#include "Engine/Camera/StereoCamera.h"

/// <summary>
/// シーンの基底クラス。MyGameがこのインターフェース越しに現在のシーンを駆動する。
/// 全シーン共通の立体視カメラ（視点別ビュー射影＋頭連動）とゲーム描画先の矩形を持つ。
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

    // ゲームの描画先矩形を設定する（クライアント座標・ピクセル。Updateの前に毎フレーム呼ぶ）。
    // ImGuiビルドではドッキングで空いた中央領域、Releaseではウィンドウ全体が渡される。
    // 各シーンは投影アスペクトやマウスピッキングの基準としてUpdateで使う。
    void SetRenderArea(float x, float y, float width, float height) {
        renderAreaX_ = x;
        renderAreaY_ = y;
        renderAreaWidth_ = width;
        renderAreaHeight_ = height;
    }

protected:

    // 立体視＋頭連動の視点別ビュー射影（各シーンのInitializeで生成し、Updateの最後に更新する）
    Engine::StereoCamera stereoCamera_;

    // ゲームの描画先矩形（クライアント座標・ピクセル）。未設定（サイズ0）の間はウィンドウ全体を使う
    float renderAreaX_ = 0.0f;
    float renderAreaY_ = 0.0f;
    float renderAreaWidth_ = 0.0f;
    float renderAreaHeight_ = 0.0f;
};
