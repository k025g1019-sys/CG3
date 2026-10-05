#pragma once

#include <cstdint>
#include <d3d12.h>

#include "Engine/Camera/Camera.h"
#include "Engine/Camera/StereoCamera.h"
#include "Engine/Light/DirectionalLight.h"
#include "Engine/Light/PointLight.h"
#include "Engine/Math/Matrix4x4.h"
#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Scene/SceneManager.h"

namespace Engine {

// シーンの切り替えは、シーンの中で ChangeScene<次のシーン>() と書く（次のフレームの頭で切り替わる）。

/// <summary>
/// シーンの基底クラス。3D描画の定型処理（カメラのビュー射影・視錐台・平行光源／点光源・
/// 立体視の視点別ビュー射影・ルートシグネチャやPSOの設定）はこのクラスが行う。
/// 派生シーンは次の On～ を実装するだけでよい。
///   OnInitialize : オブジェクトの生成・読み込み
///   OnUpdate     : ゲームの処理と、各オブジェクトの Update()
///   OnDraw       : 各オブジェクトの Draw()（標準のPSO・カメラ・ライトは設定済み）
///   OnDrawImGui  : 開発用UI（任意。Debugビルドのみ）
/// </summary>
class BaseScene {
public:

    virtual ~BaseScene() = default;

    // --- SceneManagerから呼ばれる（派生シーンは下の On～ を実装する）---

    // 光源・立体視の定数バッファを作ってから OnInitialize を呼ぶ
    void Initialize();

    // OnUpdate の後、カメラ（ビュー射影・視錐台）と光源を確定する
    void Update();

    // 標準の描画設定を行ってから OnDraw を呼ぶ。
    // viewIndex:描画する視点（立体視有効時は視点数ぶん呼ばれ、無効時は0で1回だけ呼ばれる）
    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex);

#ifdef USE_IMGUI
    void DrawImGui() { OnDrawImGui(); }
#endif

    // 視線追跡の状態を反映する（Updateの前に毎フレーム呼ぶ）。
    // enabled:視線追跡ON / gazeX,gazeY:正規化視線[-1..1]（-1左下 +1右上） / headZ:奥行き[-1..1]
    // enabledのとき、頭連動オフアクシス投影でカメラ視点をずらす（立体視OFFでも運動視差として効く）。
    void SetEyeTracking(bool enabled, float gazeX, float gazeY, float headZ) {
        stereoCamera_.SetEyeTracking(enabled, gazeX, gazeY, headZ);
    }

    // ゲームの描画先矩形を設定する（クライアント座標・ピクセル。SceneManagerが毎フレーム設定する）。
    // ImGuiビルドではドッキングで空いた中央領域、それ以外ではウィンドウ全体が渡される。
    void SetRenderArea(float x, float y, float width, float height) {
        renderAreaX_ = x;
        renderAreaY_ = y;
        renderAreaWidth_ = width;
        renderAreaHeight_ = height;
    }

protected:

    // --- 派生シーンが実装するフック ---

    // オブジェクトの生成・読み込み（シーンの開始時に1回呼ばれる）
    virtual void OnInitialize() = 0;

    // ゲームの処理と、各オブジェクトの Update（毎フレーム呼ばれる）
    virtual void OnUpdate() = 0;

    // 各オブジェクトの Draw（毎フレーム呼ばれる。立体視では視点数ぶん呼ばれる）
    virtual void OnDraw() = 0;

#ifdef USE_IMGUI
    // 開発用ImGuiウィンドウの構築（必要なシーンだけオーバーライドする）
    virtual void OnDrawImGui() {}
#endif

    // 描画に使うビュー行列。既定は camera_ のビュー行列
    // （デバッグカメラなどで差し替えるときにオーバーライドする）
    virtual Matrix4x4 CalcViewMatrix() const { return camera_.GetViewMatrix(); }

    // --- 派生シーンから使える機能 ---

    // 次のシーンを予約する（例: ChangeScene<GameScene>()。次のフレームの頭で切り替わる）
    template <class TScene>
    void ChangeScene() { SceneManager::GetInstance()->ChangeScene<TScene>(); }

    // ゲームの描画先矩形（クライアント座標・ピクセル。未設定の間はウィンドウ全体）
    float GetRenderAreaX() const;
    float GetRenderAreaY() const;
    float GetRenderAreaWidth() const;
    float GetRenderAreaHeight() const;

    // 投影に使うアスペクト比（描画先矩形の幅 / 高さ）
    float GetAspectRatio() const { return GetRenderAreaWidth() / GetRenderAreaHeight(); }

    // 直近のUpdateで確定したビュー行列・射影行列
    const Matrix4x4& GetViewMatrix() const { return view_; }
    const Matrix4x4& GetProjectionMatrix() const { return projection_; }

    // シーンのカメラ（位置・回転・画角。OnUpdateで動かすと同じフレームの描画に反映される）
    Camera camera_;

    // 平行光源（既定は真上から白色光。値を変えるとUpdateで定数バッファへ反映される）
    DirectionalLight directionalLight_{ { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, 1.0f, 1, {} };

    // 点光源（最大kMaxPointLightCount個。既定は全灯OFF）
    PointLightGroup pointLights_;

    // 立体視＋頭連動の視点別ビュー射影（Updateの最後に更新される）
    StereoCamera stereoCamera_;

private:

    ConstantBuffer<DirectionalLight> directionalLightCB_;
    ConstantBuffer<PointLightGroup> pointLightCB_;

    Matrix4x4 view_ = MakeIdentity4x4();
    Matrix4x4 projection_ = MakeIdentity4x4();

    // ゲームの描画先矩形（クライアント座標・ピクセル）。未設定（サイズ0）の間はウィンドウ全体を使う
    float renderAreaX_ = 0.0f;
    float renderAreaY_ = 0.0f;
    float renderAreaWidth_ = 0.0f;
    float renderAreaHeight_ = 0.0f;
};

} // namespace Engine
