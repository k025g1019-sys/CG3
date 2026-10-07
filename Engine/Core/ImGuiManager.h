#pragma once

#ifdef USE_IMGUI

#include <d3d12.h>
#include <wrl.h>

#include <string>
#include <vector>

struct ImDrawList;
struct ImDrawCmd;

namespace Engine {

/// <summary>
/// ImGuiの初期化・フレーム処理・描画・終了処理をまとめたシングルトン。
/// Debug・Developmentビルド限定（USE_IMGUI）。Releaseではこのクラスごとビルドから除外される。
/// 画面全体をドックスペースにし、ウィンドウを画面端へドッキングして整理できるようにする。
/// どのウィンドウにも占有されていない中央領域（＝ゲームの表示先）をGetGameArea()で公開する。
/// 画像を拡大してもにじまない点サンプリング描画（PushPointSampling）も提供する。
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

    // --- 点サンプリング描画 ---
    // ImGuiの画像描画（ImGui::Image / ImDrawList::AddImage）は既定でバイリニア補間のため、拡大するとにじむ。
    // Pushから対応するPopまでの間にdrawListへ積んだ画像は点サンプリング（最近傍）で描かれ、
    // ドット絵をくっきり拡大表示できる（ピクセル編集のキャンバス等に使う）。
    void PushPointSampling(ImDrawList* drawList);
    void PopPointSampling(ImDrawList* drawList);

private:

    ImGuiManager() = default;

    ~ImGuiManager() = default;

    ImGuiManager(const ImGuiManager&) = delete;

    ImGuiManager& operator=(const ImGuiManager&) = delete;

    // デフォルトのドッキング配置を構築する（保存済みレイアウトが無い初回起動時のみ呼ばれる）
    void BuildDefaultDockLayout(unsigned int dockspaceId, float width, float height);

    // 点サンプリング用のルートシグネチャとPSOを作る（ImGuiのDX12バックエンドと同じ構成でサンプラだけ最近傍）
    void CreatePointSamplingPipeline();

    // ImDrawListのコールバック。点サンプリング用のPSOへ切り替える（Drawの中でバックエンドから呼ばれる）
    static void PointSamplingCallback(const ImDrawList* parentList, const ImDrawCmd* cmd);

    // 初回起動時に右上／右下ノードへ配置するウィンドウ名
    std::vector<std::string> defaultDockTopRight_;
    std::vector<std::string> defaultDockBottomRight_;

    // ドッキングで空いた中央領域（BeginFrameで毎フレーム更新）
    GameArea gameArea_{};

    // --- 点サンプリング描画 ---
    Microsoft::WRL::ComPtr<ID3D12RootSignature> pointSamplingRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pointSamplingPipelineState_;
    ID3D12GraphicsCommandList* currentCommandList_ = nullptr;  // Drawの間だけ有効（コールバックが使う）
};

} // namespace Engine

#endif  // USE_IMGUI
