#include "Engine/Tools/SpriteCanvas.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <Windows.h>
#include <wincodec.h>

#include "Engine/String/ConvertString.h"
#include "externals/DirectXTex/DirectXTex.h"

// GUID_WICPixelFormat24bppBGR（JPG保存の出力形式）の実体
#pragma comment(lib, "uuid.lib")

namespace Engine {

namespace {

    // 1ピクセルのバイト数（R,G,B,A）
    constexpr size_t kBytesPerPixel = sizeof(uint32_t);

    // 0xAABBGGRR から各成分を取り出す
    constexpr uint32_t Red(uint32_t color) { return color & 0xFFu; }
    constexpr uint32_t Green(uint32_t color) { return (color >> 8) & 0xFFu; }
    constexpr uint32_t Blue(uint32_t color) { return (color >> 16) & 0xFFu; }
    constexpr uint32_t Alpha(uint32_t color) { return (color >> 24) & 0xFFu; }

    // 透明（アルファ0）どうしはRGBが違っても同じ色とみなす
    constexpr bool SameColor(uint32_t a, uint32_t b) {
        return (Alpha(a) == 0 && Alpha(b) == 0) || a == b;
    }

    // [lo, hi] へクランプする（Windows.h の min/max マクロを避けるため自前で書く）
    constexpr int Clamp(int value, int lo, int hi) {
        return value < lo ? lo : (value > hi ? hi : value);
    }

    // 四捨五入して int にする
    int RoundToInt(double value) {
        return static_cast<int>(std::lround(value));
    }

    // 幅 width の配列での (x, y) の添字
    size_t IndexOf(int x, int y, int width) {
        return static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
    }

    // 透明を白と合成して不透明にする（JPG保存用）: out = (c * a + 255 * (255 - a)) / 255
    uint32_t CompositeOverWhite(uint32_t color) {
        const uint32_t a = Alpha(color);
        const uint32_t r = (Red(color) * a + 255u * (255u - a)) / 255u;
        const uint32_t g = (Green(color) * a + 255u * (255u - a)) / 255u;
        const uint32_t b = (Blue(color) * a + 255u * (255u - a)) / 255u;
        return r | (g << 8) | (b << 16) | (0xFFu << 24);
    }

    // 浮動レイヤ floating（rect.width × rect.height）を rect の位置で dst（dstWidth × dstHeight）へ焼き込む。
    // アルファ>0 のピクセルだけ上書きし、キャンバス外にはみ出した分は捨てる
    void BurnFloating(
        std::vector<uint32_t>& dst, int dstWidth, int dstHeight,
        const std::vector<uint32_t>& floating, const SpriteCanvas::Rect& rect) {
        for (int j = 0; j < rect.height; ++j) {
            const int y = rect.y + j;
            if (y < 0 || y >= dstHeight) {
                continue;
            }
            for (int i = 0; i < rect.width; ++i) {
                const int x = rect.x + i;
                if (x < 0 || x >= dstWidth) {
                    continue;
                }
                const uint32_t color = floating[IndexOf(i, j, rect.width)];
                if (Alpha(color) > 0) {
                    dst[IndexOf(x, y, dstWidth)] = color;
                }
            }
        }
    }

    // HRESULT を "0x????????" の文字列にする（エラーメッセージ用）
    std::string HResultToString(HRESULT hr) {
        char buffer[16] = {};
        std::snprintf(buffer, sizeof(buffer), "0x%08X", static_cast<unsigned int>(hr));
        return buffer;
    }

} // namespace

// --- 基本 ---

void SpriteCanvas::Create(int width, int height) {
    width_ = Clamp(width, kMinSize, kMaxSize);
    height_ = Clamp(height, kMinSize, kMaxSize);
    pixels_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), 0);

    history_.clear();

    selecting_ = false;
    hasSelection_ = false;
    selectionMoved_ = false;
    selectionAnchor_ = Point{};
    selection_ = Rect{};
    floating_.clear();

    MarkChanged();
}

uint32_t SpriteCanvas::GetPixel(int x, int y) const {
    if (!IsInside(x, y)) {
        return 0;
    }
    return pixels_[IndexOf(x, y, width_)];
}

uint32_t SpriteCanvas::GetCompositePixel(int x, int y) const {
    if (!IsInside(x, y)) {
        return 0;
    }
    // constなのでcomposite_のキャッシュは使わず、その場で合成する
    if (hasSelection_ && selection_.Contains(x, y)) {
        const uint32_t color = floating_[IndexOf(x - selection_.x, y - selection_.y, selection_.width)];
        if (Alpha(color) > 0) {
            return color;
        }
    }
    return pixels_[IndexOf(x, y, width_)];
}

const std::vector<uint32_t>& SpriteCanvas::GetComposite() {
    if (compositeDirty_) {
        RebuildComposite();
    }
    return composite_;
}

// --- 履歴（Undo） ---

void SpriteCanvas::BeginStroke() {
    PushHistory();
}

void SpriteCanvas::Undo() {
    if (history_.empty()) {
        return;
    }
    Snapshot snapshot = std::move(history_.back());
    history_.pop_back();

    // スナップショットは「そのときの見た目」なので、選択は解除して下地だけに戻す
    selecting_ = false;
    hasSelection_ = false;
    selectionMoved_ = false;
    floating_.clear();

    width_ = snapshot.width;
    height_ = snapshot.height;
    pixels_ = std::move(snapshot.pixels);
    MarkChanged();
}

// --- ペン／消しゴム ---

void SpriteCanvas::DrawLine(Point from, Point to, int size, uint32_t color) {
    // 何も変わらなかった（キャンバス外や保護ピクセルだけ）ときは revision を進めない
    // （テクスチャの無駄な再転送を避ける）
    if (StampLine(from, to, size, color)) {
        MarkChanged();
    }
}

void SpriteCanvas::EraseLine(Point from, Point to, int size) {
    if (StampLine(from, to, size, 0)) {
        MarkChanged();
    }
}

// --- 塗りつぶし ---

void SpriteCanvas::FloodFill(Point at, uint32_t color) {
    if (!IsInside(at.x, at.y) || IsProtected(at.x, at.y)) {
        return;
    }
    // 同じ色（透明どうしも含む）なら見た目が変わらないので、履歴も積まない
    const uint32_t target = pixels_[IndexOf(at.x, at.y, width_)];
    if (SameColor(target, color)) {
        return;
    }
    PushHistory();
    Fill(at, color);
    MarkChanged();
}

void SpriteCanvas::FloodErase(Point at) {
    if (!IsInside(at.x, at.y) || IsProtected(at.x, at.y)) {
        return;
    }
    // 既に透明なら見た目が変わらないので何もしない
    const uint32_t target = pixels_[IndexOf(at.x, at.y, width_)];
    if (Alpha(target) == 0) {
        return;
    }
    PushHistory();
    Fill(at, 0);
    MarkChanged();
}

// --- 範囲選択 ---

void SpriteCanvas::BeginSelection(Point anchor) {
    // 既に選択範囲があれば、その場で確定してから新しく始める
    if (hasSelection_) {
        CommitSelection();
    }
    selecting_ = true;
    selectionAnchor_.x = Clamp(anchor.x, 0, width_ - 1);
    selectionAnchor_.y = Clamp(anchor.y, 0, height_ - 1);
    selection_ = Rect{ selectionAnchor_.x, selectionAnchor_.y, 1, 1 };
    // 見た目は変わらないので revision は進めない
}

void SpriteCanvas::UpdateSelection(Point current) {
    // ドラッグ中以外に矩形を変えると floating_ のサイズと食い違うので無視する
    if (!selecting_) {
        return;
    }
    const int cx = Clamp(current.x, 0, width_ - 1);
    const int cy = Clamp(current.y, 0, height_ - 1);
    const int x0 = cx < selectionAnchor_.x ? cx : selectionAnchor_.x;
    const int y0 = cy < selectionAnchor_.y ? cy : selectionAnchor_.y;
    const int x1 = cx > selectionAnchor_.x ? cx : selectionAnchor_.x;
    const int y1 = cy > selectionAnchor_.y ? cy : selectionAnchor_.y;
    // anchor と current を対角とする矩形（両端を含む）
    selection_ = Rect{ x0, y0, x1 - x0 + 1, y1 - y0 + 1 };
}

void SpriteCanvas::EndSelection() {
    if (!selecting_) {
        return;
    }
    selecting_ = false;

    // 矩形の中身を浮動レイヤへ移し、下地側は透明にする（見た目は変わらないので履歴はまだ積まない。
    // 最初に動かすときに MoveSelection が積む）
    floating_.assign(static_cast<size_t>(selection_.width) * static_cast<size_t>(selection_.height), 0);
    for (int j = 0; j < selection_.height; ++j) {
        const int y = selection_.y + j;
        for (int i = 0; i < selection_.width; ++i) {
            const int x = selection_.x + i;
            const size_t srcIndex = IndexOf(x, y, width_);
            floating_[IndexOf(i, j, selection_.width)] = pixels_[srcIndex];
            pixels_[srcIndex] = 0;
        }
    }

    hasSelection_ = true;
    selectionMoved_ = false;
    MarkChanged();
}

void SpriteCanvas::MoveSelection(int dx, int dy) {
    if (!hasSelection_ || (dx == 0 && dy == 0)) {
        return;
    }
    // 最初に動かすときに、動かす前の見た目（浮動レイヤが元の位置にある合成結果）を履歴へ積む。
    // 動かさずに確定した選択は見た目が変わらないので、こうすると履歴に残らない
    if (!selectionMoved_) {
        PushHistory();
    }
    // キャンバス外へはみ出してもよい（確定時に捨てる）
    selection_.x += dx;
    selection_.y += dy;
    selectionMoved_ = true;
    MarkChanged();
}

void SpriteCanvas::CommitSelection() {
    if (!hasSelection_) {
        return;
    }

    // 現在位置で下地へ焼き込む（アルファ>0 のピクセルだけ。キャンバス外は捨てる）
    BurnFloating(pixels_, width_, height_, floating_, selection_);
    hasSelection_ = false;
    floating_.clear();
    selectionMoved_ = false;

    MarkChanged();
}

// --- リサイズ ---

void SpriteCanvas::Resize(int width, int height) {
    const int newWidth = Clamp(width, kMinSize, kMaxSize);
    const int newHeight = Clamp(height, kMinSize, kMaxSize);

    // 選択中なら先に確定する（浮動レイヤはサイズ変更の対象にしない）
    if (hasSelection_) {
        CommitSelection();
    }
    selecting_ = false;

    if (newWidth == width_ && newHeight == height_) {
        return;
    }
    PushHistory();

    // 中心基準: 旧 (x, y) → 新 (x + offsetX, y + offsetY)（C++ の整数除算。はみ出した分は捨てる）
    std::vector<uint32_t> resized(static_cast<size_t>(newWidth) * static_cast<size_t>(newHeight), 0);
    const int offsetX = (newWidth - width_) / 2;
    const int offsetY = (newHeight - height_) / 2;
    for (int y = 0; y < height_; ++y) {
        const int ny = y + offsetY;
        if (ny < 0 || ny >= newHeight) {
            continue;
        }
        for (int x = 0; x < width_; ++x) {
            const int nx = x + offsetX;
            if (nx < 0 || nx >= newWidth) {
                continue;
            }
            resized[IndexOf(nx, ny, newWidth)] = pixels_[IndexOf(x, y, width_)];
        }
    }

    width_ = newWidth;
    height_ = newHeight;
    pixels_ = std::move(resized);
    MarkChanged();
}

// --- ファイル ---

bool SpriteCanvas::Save(const std::string& path, ImageFormat format, std::string& error) {
    // 見た目どおり（移動中の選択範囲を重ねた結果）を保存する
    const std::vector<uint32_t>& src = GetComposite();

    // UTF-8 のパスを wstring にしてから filesystem::path を作る（u8path は非推奨）
    const std::wstring wpath = ConvertString(path);
    const std::filesystem::path filePath(wpath);
    if (filePath.has_parent_path()) {
        // 親フォルダが無ければ作る。失敗しても保存の失敗として検知されるので続行する
        std::error_code ec;
        std::filesystem::create_directories(filePath.parent_path(), ec);
    }

    const size_t width = static_cast<size_t>(width_);
    const size_t height = static_cast<size_t>(height_);

    DirectX::Image image{};
    image.width = width;
    image.height = height;
    image.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    image.rowPitch = width * kBytesPerPixel;
    image.slicePitch = width * kBytesPerPixel * height;

    HRESULT hr = E_FAIL;
    if (format == ImageFormat::kPng) {
        // 透過をそのまま保存する（Image::pixels は非 const なので const_cast する。書き込みはされない）。
        // ピクセル値は sRGB 符号化値なので WIC_FLAGS_FORCE_SRGB で sRGB として記録する
        // （無指定だと DirectXTex が gAMA=1.0（リニア）を書き、他のアプリで開くと明るく見えてしまう）
        image.pixels = reinterpret_cast<uint8_t*>(const_cast<uint32_t*>(src.data()));
        hr = DirectX::SaveToWICFile(
            image, DirectX::WIC_FLAGS_FORCE_SRGB, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), wpath.c_str());
    } else {
        // JPG は透過を持てないので、塗っていない所を白と合成した別バッファを保存する
        std::vector<uint32_t> opaque(src.size());
        for (size_t i = 0; i < src.size(); ++i) {
            opaque[i] = CompositeOverWhite(src[i]);
        }
        image.pixels = reinterpret_cast<uint8_t*>(opaque.data());
        hr = DirectX::SaveToWICFile(
            image, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG), wpath.c_str(),
            &GUID_WICPixelFormat24bppBGR);
    }

    if (FAILED(hr)) {
        error = "Failed to save: " + path + " (HRESULT " + HResultToString(hr) + ")";
        return false;
    }
    error.clear();
    return true;
}

bool SpriteCanvas::Load(const std::string& path, std::string& error) {
    const std::wstring wpath = ConvertString(path);

    // BGR 系は RGBA8 に寄せ、sRGB メタデータは無視して値をそのまま読む（色空間変換で値が変わらないようにする）
    DirectX::ScratchImage loaded;
    DirectX::TexMetadata meta{};
    HRESULT hr = DirectX::LoadFromWICFile(
        wpath.c_str(),
        static_cast<DirectX::WIC_FLAGS>(DirectX::WIC_FLAGS_FORCE_RGB | DirectX::WIC_FLAGS_IGNORE_SRGB),
        &meta, loaded);
    if (FAILED(hr)) {
        error = "Failed to load: " + path;
        return false;
    }

    if (meta.width > static_cast<size_t>(kMaxSize) || meta.height > static_cast<size_t>(kMaxSize)) {
        error = "Image is too large (max " + std::to_string(kMaxSize) + " x " + std::to_string(kMaxSize) + ")";
        return false;
    }
    if (meta.width < static_cast<size_t>(kMinSize) || meta.height < static_cast<size_t>(kMinSize)) {
        error = "Image is empty: " + path;
        return false;
    }

    // RGBA8 以外（グレースケール・16bit など）は RGBA8 へ変換する
    const DirectX::Image* srcImage = loaded.GetImage(0, 0, 0);
    DirectX::ScratchImage converted;
    if (meta.format != DXGI_FORMAT_R8G8B8A8_UNORM) {
        hr = DirectX::Convert(
            *srcImage, DXGI_FORMAT_R8G8B8A8_UNORM,
            DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
        if (FAILED(hr)) {
            error = "Failed to convert: " + path + " (HRESULT " + HResultToString(hr) + ")";
            return false;
        }
        srcImage = converted.GetImage(0, 0, 0);
    }
    if (srcImage == nullptr || srcImage->pixels == nullptr) {
        error = "Failed to load: " + path;
        return false;
    }

    // 選択中なら先に確定し、読み込み前の見た目を履歴へ積む（読み込みも1回の編集）
    if (hasSelection_) {
        CommitSelection();
    }
    selecting_ = false;
    PushHistory();

    // キャンバスを画像サイズに合わせ、行ごとにコピーする（rowPitch にパディングがあってもよい）
    const size_t width = srcImage->width;
    const size_t height = srcImage->height;
    width_ = static_cast<int>(width);
    height_ = static_cast<int>(height);
    pixels_.assign(width * height, 0);
    const size_t rowBytes = width * kBytesPerPixel;
    for (size_t y = 0; y < height; ++y) {
        std::memcpy(pixels_.data() + y * width, srcImage->pixels + y * srcImage->rowPitch, rowBytes);
    }

    MarkChanged();
    error.clear();
    return true;
}

// --- 内部処理 ---

void SpriteCanvas::PushHistory() {
    // 「そのときの見た目」（下地＋浮動レイヤの合成結果）を積む
    Snapshot snapshot;
    snapshot.width = width_;
    snapshot.height = height_;
    snapshot.pixels = GetComposite();
    history_.push_back(std::move(snapshot));

    // kMaxUndo を超えたら古いものから捨てる
    while (static_cast<int>(history_.size()) > kMaxUndo) {
        history_.pop_front();
    }
}

bool SpriteCanvas::Stamp(Point center, int size, uint32_t color) {
    const int s = Clamp(size, kMinBrushSize, kMaxBrushSize);
    bool changed = false;

    // 左上 (cx - s/2, cy - s/2) から s×s のセルを走査し、中心から半径 s/2 の円に入るセルだけ塗る
    // （s=1 で 1 ピクセル、s=2 で 2×2、s=3 で 3×3、大きくなると円になる）
    const int left = center.x - s / 2;
    const int top = center.y - s / 2;
    const double half = s * 0.5;
    const double radiusSq = half * half + 1e-4;

    for (int j = 0; j < s; ++j) {
        const int y = top + j;
        if (y < 0 || y >= height_) {
            continue;
        }
        const double dy = (j + 0.5) - half;
        for (int i = 0; i < s; ++i) {
            const int x = left + i;
            if (x < 0 || x >= width_) {
                continue;
            }
            const double dx = (i + 0.5) - half;
            if (dx * dx + dy * dy > radiusSq) {
                continue;
            }
            if (IsProtected(x, y)) {
                continue;
            }
            uint32_t& pixel = pixels_[IndexOf(x, y, width_)];
            if (pixel != color) {
                pixel = color;
                changed = true;
            }
        }
    }
    return changed;
}

bool SpriteCanvas::StampLine(Point from, Point to, int size, uint32_t color) {
    const int s = Clamp(size, kMinBrushSize, kMaxBrushSize);

    if (from.x == to.x && from.y == to.y) {
        return Stamp(from, s, color);
    }
    bool changed = false;

    // 長い方の軸のピクセル数を分割数にし、太さに応じた間隔でスタンプを並べる（隙間が出ない範囲で間引く）
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    const int adx = dx < 0 ? -dx : dx;
    const int ady = dy < 0 ? -dy : dy;
    const int n = adx > ady ? adx : ady;
    const int step = (s / 4) < 1 ? 1 : (s / 4);

    for (int t = 0; t < n; t += step) {
        Point p;
        p.x = from.x + RoundToInt(static_cast<double>(dx) * t / n);
        p.y = from.y + RoundToInt(static_cast<double>(dy) * t / n);
        changed = Stamp(p, s, color) || changed;
    }
    // 終点には必ず置く（間引きで飛んでも線が途切れないようにする）
    changed = Stamp(to, s, color) || changed;
    return changed;
}

void SpriteCanvas::Fill(Point at, uint32_t color) {
    if (!IsInside(at.x, at.y) || IsProtected(at.x, at.y)) {
        return;
    }
    const uint32_t target = pixels_[IndexOf(at.x, at.y, width_)];

    // 4連結の塗りつぶし。再帰はせず、明示的なスタックで広げる（大きなキャンバスでもスタックを溢れさせない）
    std::vector<uint8_t> visited(pixels_.size(), 0);
    std::vector<Point> stack;
    stack.push_back(at);
    visited[IndexOf(at.x, at.y, width_)] = 1;

    while (!stack.empty()) {
        const Point p = stack.back();
        stack.pop_back();
        pixels_[IndexOf(p.x, p.y, width_)] = color;

        const Point neighbors[4] = {
            Point{ p.x + 1, p.y },
            Point{ p.x - 1, p.y },
            Point{ p.x, p.y + 1 },
            Point{ p.x, p.y - 1 },
        };
        for (const Point& next : neighbors) {
            // 範囲外・選択範囲内（保護）で止まる
            if (!IsInside(next.x, next.y) || IsProtected(next.x, next.y)) {
                continue;
            }
            const size_t index = IndexOf(next.x, next.y, width_);
            if (visited[index]) {
                continue;
            }
            if (!SameColor(pixels_[index], target)) {
                continue;
            }
            visited[index] = 1;
            stack.push_back(next);
        }
    }
}

bool SpriteCanvas::IsProtected(int x, int y) const {
    return hasSelection_ && selection_.Contains(x, y);
}

void SpriteCanvas::RebuildComposite() {
    // 下地の上に、選択中なら浮動レイヤを現在位置へ重ねる
    composite_ = pixels_;
    if (hasSelection_) {
        BurnFloating(composite_, width_, height_, floating_, selection_);
    }
    compositeDirty_ = false;
}

void SpriteCanvas::MarkChanged() {
    compositeDirty_ = true;
    ++revision_;
}

} // namespace Engine
