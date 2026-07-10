#pragma once

#ifdef USE_IMGUI

#include <d3d12.h>

#include <string>
#include <vector>

namespace Engine {

/// <summary>
/// ImGuiの初期化・フレーム処理・描画・終了処理をまとめたシングルトン。
/// Debugビルド限定（USE_IMGUI）。Releaseではこのクラスごとビルドから除外される。
/// 画面全体をドックスペースにし、ウィンドウを画面端へドッキングして整理できるようにする。
/// どのウィンドウにも占有されていない中央領域（＝ゲームの表示先）をGetGameArea()で公開する。
/// </summary>
class ImGuiManager {
public:

    // ゲーム表示に使える領域（クライアント座標・ピクセル）
    struct GameArea {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    static ImGuiManager* GetInstance();

    // ImGuiコンテキストとWin32/DX12バックエンドを初期化する
    // （WinApp・DirectXCore・DescriptorHeapManagerの初期化後に呼ぶ）
    void Initialize();

    // 保存済みレイアウトが無い初回起動時に組むデフォルトドッキング配置を登録する
    // （最初のBeginFrameより前に呼ぶ。左＝ゲーム、右上・右下に各ウィンドウ名で配置する）。
    void SetDefaultDockLayout(
        std::vector<std::string> topRight, std::vector<std::string> bottomRight);

    // フレーム開始（この後にImGui::Begin等でUIを構築する）
    void BeginFrame();

    // UI構築の終了（描画データを確定する）
    void Render();

    // 確定済みの描画データをコマンドリストへ積む（シーン描画の後に呼ぶ）
    void Draw(ID3D12GraphicsCommandList* commandList);

    // 終了処理（SRVヒープ解放より前に呼ぶ）
    void Finalize();

    // ドッキングで空いた中央領域＝ゲームの表示先（BeginFrame後に有効）
    GameArea GetGameArea() const { return gameArea_; }

private:

    ImGuiManager() = default;

    ~ImGuiManager() = default;

    ImGuiManager(const ImGuiManager&) = delete;

    ImGuiManager& operator=(const ImGuiManager&) = delete;

    // デフォルトのドッキング配置を構築する（保存済みレイアウトが無い初回起動時のみ呼ばれる）
    void BuildDefaultDockLayout(unsigned int dockspaceId, float width, float height);

    // 初回起動時に右上／右下ノードへ配置するウィンドウ名
    std::vector<std::string> defaultDockTopRight_;
    std::vector<std::string> defaultDockBottomRight_;

    // ドッキングで空いた中央領域（BeginFrameで毎フレーム更新）
    GameArea gameArea_{};
};

} // namespace Engine

#endif  // USE_IMGUI
