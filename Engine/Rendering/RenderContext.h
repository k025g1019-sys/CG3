#pragma once

#include <d3d12.h>

#include "Engine/Culling/FrustumCulling.h"

namespace Engine {

/// <summary>
/// 現在のフレームの描画情報（視錐台・ゲームの描画先サイズ・描画中の視点のビュー射影CBV）を持つシングルトン。
/// BaseScene が毎フレーム設定し、Object3D・Sprite などが参照する。
/// これにより、ゲーム側のコードは視錐台やコマンドリストを持ち回らずに Update() / Draw() と書ける。
/// </summary>
class RenderContext {
public:

    static RenderContext* GetInstance();

    // 描画コマンドを積むコマンドリスト（DirectXCoreのもの）
    ID3D12GraphicsCommandList* GetCommandList() const;

    // --- 視錐台カリング（中心カメラの視錐台。BaseSceneがUpdateの最後に設定する）---
    void SetFrustum(const Frustum3D& frustum) {
        frustum_ = frustum;
        hasFrustum_ = true;
    }
    void ClearFrustum() { hasFrustum_ = false; }
    // 視錐台が設定されているか（未設定の間はカリングせずに全て描画する）
    bool HasFrustum() const { return hasFrustum_; }
    const Frustum3D& GetFrustum() const { return frustum_; }

    // --- ゲームの描画先サイズ（ピクセル。スプライトの正射影・画面外判定に使う）---
    void SetScreenSize(float width, float height) {
        screenWidth_ = width;
        screenHeight_ = height;
    }
    float GetScreenWidth() const { return screenWidth_; }
    float GetScreenHeight() const { return screenHeight_; }

    // --- 描画中の視点のビュー射影CBV（BaseSceneが視点ごとのDrawの最初に設定する）---
    // スプライトなど、ビュー射影を一時的に差し替えた描画が元に戻すために使う。
    void SetViewProjectionAddress(D3D12_GPU_VIRTUAL_ADDRESS address) { viewProjectionAddress_ = address; }
    D3D12_GPU_VIRTUAL_ADDRESS GetViewProjectionAddress() const { return viewProjectionAddress_; }

private:

    RenderContext();
    ~RenderContext() = default;

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;

private:

    Frustum3D frustum_{};
    bool hasFrustum_ = false;

    float screenWidth_ = 0.0f;
    float screenHeight_ = 0.0f;

    D3D12_GPU_VIRTUAL_ADDRESS viewProjectionAddress_ = 0;
};

} // namespace Engine
