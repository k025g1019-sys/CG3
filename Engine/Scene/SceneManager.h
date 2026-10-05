#pragma once

#include <cstdint>
#include <d3d12.h>
#include <memory>

namespace Engine {

class BaseScene;

//   ・シーンの追加     : Engine::BaseScene を継承したクラスをゲーム側に作るだけ（登録は不要）
//   ・シーンの切り替え : ChangeScene<GameScene>() のように呼ぶ（シーンの中からでも、MyGameからでもよい）

/// <summary>
/// 現在のシーンを持ち、シーンの切り替えを行うシングルトン。
/// エンジンは個々のシーンを知らない。どんなシーンを作るか・いつ切り替えるかはゲーム側が決める。
/// 切り替えは予約制で、次のフレームの頭で行われる（Update中に呼んでも実行中のシーンは壊れない）。
/// </summary>
class SceneManager {
public:

    static SceneManager* GetInstance();

    // 次のシーンを予約する（切り替えは次のフレームの頭。同じフレームに複数回呼んだら最後の予約が有効）
    void ChangeScene(std::unique_ptr<BaseScene> nextScene);

    // 次のシーンを型で予約する（例: ChangeScene<GameScene>()）
    template <class TScene>
    void ChangeScene() { ChangeScene(std::make_unique<TScene>()); }

    // 現在のシーン（まだ無ければnullptr）
    BaseScene* GetCurrentScene() const { return currentScene_.get(); }

    // --- 以下はFrameworkから呼ばれる ---

    // 予約があればシーンを切り替える（GPUの完了待ち → 前のシーンを破棄 → 次のシーンを初期化）
    void ApplySceneChange();

    // ゲームの描画先矩形（クライアント座標・ピクセル）を設定し、現在のシーンへ伝える
    void SetRenderArea(float x, float y, float width, float height);

    void Update();

    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex);

#ifdef USE_IMGUI
    void DrawImGui();
#endif

    // 現在のシーンと予約中のシーンを破棄する（エンジンの終了処理より前に呼ぶ）
    void Finalize();

private:

    SceneManager() = default;
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

private:

    std::unique_ptr<BaseScene> currentScene_;
    std::unique_ptr<BaseScene> nextScene_;

    // ゲームの描画先矩形（切り替え直後のシーンにも同じ値を渡す）
    float renderAreaX_ = 0.0f;
    float renderAreaY_ = 0.0f;
    float renderAreaWidth_ = 0.0f;
    float renderAreaHeight_ = 0.0f;
};

} // namespace Engine
