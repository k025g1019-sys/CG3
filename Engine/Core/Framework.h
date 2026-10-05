#pragma once

#include <cstdint>
#include <d3d12.h>
#include <string>

#include "Engine/Scene/SceneManager.h"

namespace Engine {

/// <summary>
/// アプリケーション全体の骨組み。
/// エンジン各サブシステムの初期化〜メインループ〜終了処理を担当する。
/// 毎フレーム、現在のシーン（SceneManager）を更新・描画する。
/// 立体視（StereoRenderer）が有効なときは視点数ぶんDrawを呼んでオフスクリーンへ描画し、
/// 合成パスでバックバッファへ出力する。無効なときは従来どおり1回だけ直接描画する。
/// ImGuiビルドではゲームの描画先をドッキングで空いた中央領域に合わせる（ゲームがUIに隠れない）。
/// ゲーム側はこのクラスを継承し、Initialize でタイトルと最初のシーンを決めるだけでよい。
/// </summary>
class Framework {
public:

    virtual ~Framework() = default;

    // 初期化 → メインループ → 終了処理 まで一括で実行する
    void Run();

protected:

    // エンジン各サブシステムの初期化。
    // 派生クラスはこれを呼んだ後に自身の初期化（最初のシーンの予約など）を行う。
    virtual void Initialize();

    // 終了処理。派生クラスは自身のリソースを解放してからこれを呼ぶ。
    virtual void Finalize();

    // 毎フレームの更新処理（既定では現在のシーンを更新する）
    virtual void Update();

    // 描画コマンドの発行（既定では現在のシーンを描画する）。
    // viewIndex:描画する視点。立体視有効時は視点数ぶん呼ばれ、無効時は0で1回だけ呼ばれる。
    virtual void Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex);

    // シーン描画の前に毎フレーム呼ばれるフック（テクスチャ転送コマンドの発行など。既定では何もしない）
    virtual void PreDraw(ID3D12GraphicsCommandList* commandList) { (void)commandList; }

#ifdef USE_IMGUI
    // 開発用ImGuiウィンドウの構築（既定では現在のシーンのImGuiを構築する。Debug・Developmentビルドのみ呼ばれる）
    virtual void DrawImGui();
#endif

    // ウィンドウのタイトルを設定する（Initializeより前に呼べば最初からそのタイトルで開く）
    void SetWindowTitle(const std::wstring& title);

    // 次のシーンを予約する（例: ChangeScene<TitleScene>()。最初のシーンもこれで指定する）
    template <class TScene>
    void ChangeScene() { SceneManager::GetInstance()->ChangeScene<TScene>(); }
};

} // namespace Engine
