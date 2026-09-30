#pragma once

#include <cstdint>
#include <d3d12.h>
#include <string>

namespace Engine {

/// <summary>
/// アプリケーション全体の骨組み。
/// エンジン各サブシステムの初期化〜メインループ〜終了処理を担当する。
/// 立体視（StereoRenderer）が有効なときは視点数ぶんDrawを呼んでオフスクリーンへ描画し、
/// 合成パスでバックバッファへ出力する。無効なときは従来どおり1回だけ直接描画する。
/// ImGuiビルドではゲームの描画先をドッキングで空いた中央領域に合わせる（ゲームがUIに隠れない）。
/// ゲーム側はこのクラスを継承し、Update / Draw（/ DrawImGui）を実装する。
/// </summary>
class Framework {
public:

    virtual ~Framework() = default;

    // 初期化 → メインループ → 終了処理 まで一括で実行する
    void Run();

protected:

    // エンジン各サブシステムの初期化。
    // 派生クラスはこれを呼んだ後に自身の初期化（シーン生成など）を行う。
    virtual void Initialize();

    // 終了処理。派生クラスは自身のリソースを解放してからこれを呼ぶ。
    virtual void Finalize();

    // 毎フレームの更新処理
    virtual void Update() = 0;

    // 描画コマンドの発行。
    // viewIndex:描画する視点。立体視有効時は視点数ぶん呼ばれ、無効時は0で1回だけ呼ばれる。
    virtual void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) = 0;

    // シーン描画の前に毎フレーム呼ばれるフック（テクスチャ転送コマンドの発行など。既定では何もしない）
    virtual void PreDraw(ID3D12GraphicsCommandList* commandList) { (void)commandList; }

#ifdef USE_IMGUI
    // 開発用ImGuiウィンドウの構築（Debugビルドのみ呼ばれる）
    virtual void DrawImGui() = 0;
#endif

    // ウィンドウのタイトルを設定する（Initializeより前に呼べば最初からそのタイトルで開く）
    void SetWindowTitle(const std::wstring& title);
};

} // namespace Engine
