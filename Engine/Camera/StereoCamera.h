#pragma once

#include <cstdint>
#include <d3d12.h>

#include "Engine/Math/Matrix4x4.h"
#include "Engine/Rendering/ConstantBuffer.h"

/// <summary>
/// 立体視＋頭連動オフアクシスの「視点別ビュー射影」を管理するカメラ補助。
/// 中心カメラのビュー・射影から各視点（眼）のビュー射影を作り、視点ごとのCBuffer（VSのb1）へ書き込む。
/// ・視点は中心カメラの右方向へ平行にずらす（toe-in不使用）＋オフアクシス射影（収束面で視差ゼロ）
/// ・視線追跡ONのときは頭位置オフセットも同じ仕組みに乗せる（運動視差。立体視OFF＝1視点でも効く）
/// </summary>
class StereoCamera {
public:

    // 視点別CBufferを生成する（スロットは [フレームスロット][視点] の2次元）
    void Initialize(ID3D12Device* device);

    // 視線追跡の状態を反映する（Updateの前に呼ぶ）。
    // enabled:視線追跡ON / gazeX,gazeY:正規化視線[-1..1]（-1左下 +1右上） / headZ:奥行き[-1..1]
    void SetEyeTracking(bool enabled, float gazeX, float gazeY, float headZ) {
        eyeTrackingEnabled_ = enabled;
        gazeX_ = gazeX;
        gazeY_ = gazeY;
        gazeZ_ = headZ;
    }

    // 中心カメラのビュー・射影から各視点のビュー射影を計算し、
    // 現在のフレームスロットのCBufferへ書き込む（毎フレーム、シーンのUpdateの最後に呼ぶ）
    void Update(const Matrix4x4& view, const Matrix4x4& projection);

    // 指定フレームスロット・視点のビュー射影CBufferのGPUアドレス（VSのb1へバインドする）
    D3D12_GPU_VIRTUAL_ADDRESS GetViewProjectionAddress(uint32_t frameIndex, uint32_t viewIndex) const;

#ifdef USE_IMGUI
    // 立体視パラメータと頭連動の調整UI（呼び出し側のウィンドウ内へ差し込む。Begin/Endは行わない）
    void DrawImGuiSection();
#endif

private:

    // 視点ごとのビュー射影CBuffer。
    // スロットは [フレームスロット][視点] の2次元（index = frame * kMaxViewCount + view）。
    ConstantBuffer<Matrix4x4> viewProjectionCB_;

    // 共有ステレオパラメータ（ImGuiで調整）。視点描画に効く。
    float eyeSeparation_ = 0.1f;   // 眼間距離（ワールド単位）
    float convergence_ = 10.0f;    // 収束距離（視差ゼロ面までの距離。カメラ→被写体の距離が目安）

    // --- 視線追跡（頭連動オフアクシス）---
    // 視線位置を擬似的な頭位置とみなし、カメラを左右/上下へ平行移動＋射影を水平/垂直シアーする
    // （収束面を固定したまま「窓の中を覗き込む」3DS風の運動視差。物体を立体的な角度から見やすくする）。
    bool eyeTrackingEnabled_ = false;
    float gazeX_ = 0.0f;  // 正規化視線（-1左/+1右）
    float gazeY_ = 0.0f;  // 正規化視線（-1下/+1上）
    float gazeZ_ = 0.0f;  // 正規化奥行き（-1後/+1前。未使用）
    float gazeMoveScaleX_ = 1.2f;  // 視線が端のときの水平移動量（ワールド単位）
    float gazeMoveScaleY_ = 0.7f;  // 視線が端のときの垂直移動量（ワールド単位）
};
