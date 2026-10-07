#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace Engine {

/// <summary>
/// スプライトエディタのドキュメント（RGBA8のピクセル配列と編集操作）。
/// ペン・塗りつぶし・消しゴム・スポイト・範囲選択と移動・中心基準のリサイズ・Undo（直近5回）・
/// PNG/JPGの保存と読み込みをまとめる。ImGuiやGPUには依存しない（表示と入力はSpriteEditorが担当する）。
///
/// ピクセルは uint32_t 1個で、メモリ順 R,G,B,A（リトルエンディアンで 0xAABBGGRR。ImGuiのIM_COL32と同じ並び）。
/// アルファ0を「塗っていない（透明）」として扱う。色のグラデーションはなく、ペンは色をそのまま置く。
///
/// 範囲選択は「下地」とは別の浮動レイヤとして持ち、Enter（CommitSelection）で下地へ焼き込む。
/// 選択中はペン・消しゴム・塗りつぶしは選択範囲の外にだけ効く。
/// Undoのスナップショットは「そのときの見た目（合成結果）」で、Undoすると選択も解除される。
/// </summary>
class SpriteCanvas {
public:

    static constexpr int kMinSize = 1;         // キャンバスの最小サイズ（ピクセル）
    static constexpr int kMaxSize = 1024;      // キャンバスの最大サイズ（ピクセル）
    static constexpr int kMinBrushSize = 1;    // ペン／消しゴムの最小の太さ
    static constexpr int kMaxBrushSize = 100;  // ペン／消しゴムの最大の太さ
    static constexpr int kMaxUndo = 5;         // Undoで戻せる回数

    // キャンバス座標のピクセル位置（左上原点。範囲外の値も渡せる）
    struct Point {
        int x = 0;
        int y = 0;
    };

    // キャンバス座標の矩形（左上とサイズ）
    struct Rect {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        bool Contains(int px, int py) const {
            return px >= x && py >= y && px < x + width && py < y + height;
        }
    };

    // 保存形式
    enum class ImageFormat {
        kPng,  // 透過をそのまま保存する
        kJpg,  // 塗っていない所は白として保存する
    };

    // --- 基本 ---

    // 指定サイズの透明なキャンバスを作る（履歴と選択もクリアする）。サイズはkMinSize〜kMaxSizeへクランプされる
    void Create(int width, int height);

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

    bool IsInside(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }

    // 下地のピクセル（範囲外は0）
    uint32_t GetPixel(int x, int y) const;

    // 見た目どおりのピクセル（移動中の選択範囲を重ねた結果。範囲外は0）。スポイトに使う
    uint32_t GetCompositePixel(int x, int y) const;

    // 見た目どおりの合成結果（下地＋移動中の選択範囲。選択が無ければ下地と同じ）。幅×高さ個
    const std::vector<uint32_t>& GetComposite();

    // 内容が変わるたびに増えるカウンタ（テクスチャの更新が必要かの判定に使う）
    uint64_t GetRevision() const { return revision_; }

    // --- 履歴（Undo） ---

    // ひと続きの編集（ペン／消しゴムのドラッグ）を始める前に呼ぶ。現在の見た目を履歴へ積む
    void BeginStroke();

    bool CanUndo() const { return !history_.empty(); }

    // 直近の編集を取り消す（選択中なら選択は解除される）
    void Undo();

    // --- ペン／消しゴム ---
    // fromからtoまで線分補間しながら、太さsizeの円形スタンプで塗る（from==toなら1点）。
    // 事前にBeginStrokeを呼ぶこと。選択中は選択範囲の外にだけ効く

    // ペン（colorをそのまま置く）
    void DrawLine(Point from, Point to, int size, uint32_t color);

    // 消しゴム（アルファ0にする）
    void EraseLine(Point from, Point to, int size);

    // --- 塗りつぶし（1回の編集として履歴に積まれる。選択中は選択範囲の外にだけ効く） ---

    // atと同じ色の連結領域（上下左右）をcolorで塗る。atが範囲外なら何もしない
    void FloodFill(Point at, uint32_t color);

    // atと同じ色の連結領域をアルファ0にする（透過塗りつぶし）
    void FloodErase(Point at);

    // --- 範囲選択 ---

    // 選択範囲（確定前の浮動レイヤ）があるか
    bool HasSelection() const { return hasSelection_; }

    // 矩形をドラッグで決めている最中か
    bool IsSelecting() const { return selecting_; }

    // 選択範囲の現在位置（移動後。HasSelectionかIsSelectingのとき有効）
    Rect GetSelection() const { return selection_; }

    // Ctrl+左ドラッグの開始（既に選択範囲があればCommitSelectionしてから新しく始める）
    void BeginSelection(Point anchor);

    // ドラッグ中（anchorとcurrentを対角とする矩形。キャンバス内にクランプ）
    void UpdateSelection(Point current);

    // ドラッグ終了。矩形の中身を浮動レイヤへ切り出す（見た目は変わらないので履歴はまだ積まない）
    void EndSelection();

    // 枠と中身を一緒に動かす（キャンバス外へはみ出してもよい。はみ出した分は確定時に捨てられる）。
    // 最初に動かしたときに、動かす前の見た目を履歴へ積む（動かさずに確定すれば履歴に残らない）
    void MoveSelection(int dx, int dy);

    // 現在位置で下地へ焼き込み、選択を解除する（Enter）
    void CommitSelection();

    // --- リサイズ ---

    // 中心を基準にサイズを変える（はみ出した部分は捨て、足りない部分は透明）。
    // 選択中なら先に確定し、1回の編集として履歴へ積む。サイズはkMinSize〜kMaxSizeへクランプされる
    void Resize(int width, int height);

    // --- ファイル ---

    // 見た目どおりの内容を保存する（キャンバスサイズ＝画像サイズ。親フォルダが無ければ作る）。
    // PNGは透過のまま、JPGは塗っていない所を白にして保存する。失敗時はfalseを返しerrorに理由を入れる
    bool Save(const std::string& path, ImageFormat format, std::string& error);

    // 画像ファイルを読み込んでキャンバスを置き換える（サイズも画像に合わせる。1回の編集として履歴へ積む）。
    // kMaxSizeより大きい画像は読み込まずfalseを返す。選択中なら先に確定する
    bool Load(const std::string& path, std::string& error);

private:

    // Undo用スナップショット（そのときの見た目）
    struct Snapshot {
        int width = 0;
        int height = 0;
        std::vector<uint32_t> pixels;
    };

    // 現在の見た目を履歴へ積む（kMaxUndoを超えたら古いものから捨てる）
    void PushHistory();

    // 太さsizeの円形スタンプをcenterに置く（colorをそのまま書く。消しゴムは0）。
    // 1ピクセルでも値が変わればtrue（範囲外や保護ピクセルだけに当たったときはfalse）
    bool Stamp(Point center, int size, uint32_t color);

    // 2点間を補間しながらStampする。1ピクセルでも値が変わればtrue
    bool StampLine(Point from, Point to, int size, uint32_t color);

    // atと同色の連結領域をcolorで置き換える（範囲外・選択範囲内は無視）
    void Fill(Point at, uint32_t color);

    // 選択中かつ選択範囲内なら true（ペン・消しゴムが効かない場所）
    bool IsProtected(int x, int y) const;

    // 合成結果を作り直す
    void RebuildComposite();

    // 内容が変わったことを記録する（revision_を進め、合成結果を無効化する）
    void MarkChanged();

private:

    int width_ = 0;
    int height_ = 0;

    std::vector<uint32_t> pixels_;  // 下地（幅×高さ）

    // --- 範囲選択（浮動レイヤ） ---
    bool selecting_ = false;      // ドラッグで矩形を決めている最中
    bool hasSelection_ = false;   // 切り出し済みの浮動レイヤがある
    bool selectionMoved_ = false; // 切り出し後に一度でも動かしたか
    Point selectionAnchor_{};     // ドラッグ開始点
    Rect selection_{};            // 現在の矩形（移動後）
    std::vector<uint32_t> floating_;  // 切り出した中身（selection_.width × selection_.height）

    // --- 履歴 ---
    std::deque<Snapshot> history_;  // 古い順。末尾が直近

    // --- 合成結果のキャッシュ ---
    std::vector<uint32_t> composite_;
    bool compositeDirty_ = true;
    uint64_t revision_ = 0;
};

} // namespace Engine
