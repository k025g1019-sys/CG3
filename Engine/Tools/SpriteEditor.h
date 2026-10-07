#pragma once

#ifdef USE_IMGUI

#include <cstdint>
#include <string>
#include <vector>

#include "Engine/Tools/SpriteCanvas.h"

struct ImVec2;

namespace Engine {

/// <summary>
/// ImGuiの「Sprite Editor」ウィンドウ。マウスで2Dスプライトを描き、動的テクスチャ経由でゲーム内のSpriteへ
/// 即時に反映し、気に入ったらPNG/JPGへ保存する。既存のPNGを読み込んで描き直すこともできる。
/// Debug・Developmentビルド限定（USE_IMGUI）。Releaseではこのクラスごとビルドから除外される。
///
/// Frameworkが Initialize / DrawImGui を呼ぶので、ゲーム側は
///   sprite.SetTextureHandle(SpriteEditor::GetInstance()->GetTextureHandle());
/// で描いた内容をSpriteに表示し、SetOpen(true) でウィンドウを開くだけでよい。
/// 操作一覧は docs/SpriteEditor.md を参照。
/// </summary>
class SpriteEditor {
public:

    static SpriteEditor* GetInstance();

    // キャンバス（64×64・透明）と、その内容を映す動的テクスチャを作る
    // （TextureManager::Initializeの後に呼ぶ。Frameworkが呼ぶ）
    void Initialize();

    // ウィンドウの構築・マウスとキーボードの処理・テクスチャの更新を行う（毎フレーム。Frameworkが呼ぶ）。
    // ウィンドウが閉じていても、未反映のキャンバス内容はテクスチャへ反映する
    void DrawImGui();

    // ウィンドウを開く／閉じる（既定は閉じている）
    void SetOpen(bool open) { open_ = open; }
    bool IsOpen() const { return open_; }

    // キャンバス内容のテクスチャハンドル（Sprite::SetTextureHandleに渡す。Initialize後に有効）
    uint32_t GetTextureHandle() const { return textureHandle_; }

    // キャンバスの現在のサイズ（ピクセル。リサイズや読み込みで変わる）
    int GetCanvasWidth() const { return canvas_.GetWidth(); }
    int GetCanvasHeight() const { return canvas_.GetHeight(); }

private:

    SpriteEditor() = default;

    ~SpriteEditor() = default;

    SpriteEditor(const SpriteEditor&) = delete;

    SpriteEditor& operator=(const SpriteEditor&) = delete;

    // マウス操作の種類（ボタンを押してから離すまで続く）。
    // スポイト（Alt＋左クリック）は押した瞬間に完了するので状態を持たない
    enum class Tool {
        kNone,
        kPen,     // 左ドラッグ
        kEraser,  // 右ドラッグ
        kSelect,  // 左Ctrl＋左ドラッグ（矩形を決める）
        kMove,    // 選択範囲内の左ドラッグ（枠と中身を動かす）
        kPan,     // ホイール押し込みドラッグ（キャンバスの表示位置を動かす）
    };

    // 左側のパネル（色・パレット・太さ・表示・サイズ・ファイル）
    void DrawToolPanel();

    // ツールパネルとキャンバスの間の仕切り（ドラッグでツールパネルの幅を変える）
    void DrawSplitter();

    // 右側のキャンバス表示と、キャンバス上のマウス操作
    void DrawCanvasView();

    // キャンバス上のマウス操作（DrawCanvasViewから呼ぶ。pixelはカーソル位置のキャンバス座標）
    void HandleCanvasMouse(bool hovered, SpriteCanvas::Point pixel);

    // キーボードショートカット（Ctrl+Z・Enter）とホイール（太さ変更）
    void HandleShortcuts(bool canvasHovered);

    // Alt＋ホイールで表示倍率を変える（カーソルの下のピクセルが動かないようスクロール位置も補正する）。
    // キャンバスの子ウィンドウの中で、キャンバスを置く前に呼ぶ（originはキャンバス左上の画面座標）
    void HandleZoomWheel(const ImVec2& origin);

    // 保存（上書き確認の後に呼ばれる）
    void SaveToFile();

    // 読み込み
    void LoadFromFile();

    // パス欄の拡張子とフォーマット選択を揃える（拡張子が.png/.jpg/.jpegならフォーマットをそれに合わせ、無ければ付け足す）
    void SyncPathAndFormat();

    // キャンバスの内容が変わっていればテクスチャへ反映する（サイズが変わっていれば作り直す）
    void UploadTextureIfChanged();

    // 現在のペン色を0xAABBGGRRへ
    uint32_t GetPenColorU32() const;

private:

    SpriteCanvas canvas_;

    uint32_t textureHandle_ = 0;
    uint64_t uploadedRevision_ = 0;  // 最後にテクスチャへ反映したキャンバスのrevision
    int uploadedWidth_ = 0;          // 反映済みテクスチャのサイズ
    int uploadedHeight_ = 0;

    bool open_ = false;

    // --- レイアウト ---
    float toolPanelWidth_ = 280.0f;  // 左側のツールパネルの幅（画面ピクセル。仕切りのドラッグで変わる。起動時は既定に戻る）

    // --- 色 ---
    float penColor_[4] = { 0.0f, 0.0f, 0.0f, 1.0f };  // ColorPickerで編集するRGBA（0〜1）
    std::vector<uint32_t> palette_;                   // 登録色（0xAABBGGRR）

    // --- 太さ・表示 ---
    int penSize_ = 4;
    int eraserSize_ = 4;
    int zoom_ = 8;           // キャンバス1ピクセルを何画面ピクセルで表示するか
    bool showGrid_ = true;   // 拡大時にピクセルの境界線を描く

    // --- キャンバスサイズ変更の入力欄 ---
    int resizeWidth_ = 64;
    int resizeHeight_ = 64;

    // --- ファイル ---
    char pathBuffer_[260] = "resources/sprite.png";  // 実行ディレクトリからの相対パス（サブフォルダ可）
    int formatIndex_ = 0;                            // 0:PNG / 1:JPG
    std::string statusMessage_;                      // 直近の保存／読み込みの結果

    // --- マウス操作の状態 ---
    Tool tool_ = Tool::kNone;
    SpriteCanvas::Point lastPixel_{};  // 直前のフレームのカーソル位置（線分補間・移動量の基準）
};

} // namespace Engine

#endif  // USE_IMGUI
