#ifdef USE_IMGUI

#include "Engine/Tools/SpriteEditor.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

#include "Engine/Core/ImGuiManager.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/String/ConvertString.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"  // ImGui::SetItemKeyOwner（ホイールの所有権）

namespace Engine {

namespace {

    // 動的テクスチャの識別名（実ファイルパスと衝突しないよう "<>" で囲む）
    constexpr const char* kTextureKey = "<sprite-editor>";

    // 起動時のキャンバスサイズ（ピクセル）
    constexpr int kInitialCanvasSize = 64;

    // 左側ツールパネルの幅の範囲（画面ピクセル）と、仕切りの太さ・キャンバス表示に最低限残す幅
    constexpr float kMinToolPanelWidth = 150.0f;
    constexpr float kSplitterWidth = 8.0f;
    constexpr float kMinCanvasViewWidth = 120.0f;

    // 表示倍率の範囲（キャンバス1ピクセルを画面何ピクセルで表示するか）
    constexpr int kMinZoom = 1;
    constexpr int kMaxZoom = 32;
    // Fitボタンが立てる要求値。同じフレームのDrawCanvasViewが表示領域に収まる倍率へ置き換える
    // （表示領域の大きさはキャンバス側の子ウィンドウでしか分からないため）
    constexpr int kZoomFitRequest = 0;
    // グリッド線を描く最小倍率（これより小さいと線でピクセルが埋まる）
    constexpr int kGridMinZoom = 6;

    // 透明の下地に敷く市松模様（画面ピクセル単位のセル）
    constexpr float kCheckerCellSize = 8.0f;
    constexpr ImU32 kCheckerLight = IM_COL32(200, 200, 200, 255);
    constexpr ImU32 kCheckerDark = IM_COL32(140, 140, 140, 255);
    constexpr ImU32 kGridColor = IM_COL32(0, 0, 0, 40);
    constexpr ImU32 kFrameBlack = IM_COL32(0, 0, 0, 255);
    constexpr ImU32 kFrameWhite = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 kBrushFrameWhite = IM_COL32(255, 255, 255, 180);

    // パレット
    constexpr size_t kMaxPaletteColors = 64;
    constexpr float kPaletteButtonSize = 20.0f;
    constexpr float kCurrentColorButtonSize = 40.0f;

    // 保存形式（Comboの項目順＝formatIndex_）
    constexpr const char* kFormatItems[] = { "PNG", "JPG" };
    constexpr const char* kFormatExtensions[] = { ".png", ".jpg" };

    int Clamp(int value, int low, int high) {
        return value < low ? low : (value > high ? high : value);
    }

    float MinF(float a, float b) { return a < b ? a : b; }
    float MaxF(float a, float b) { return a > b ? a : b; }
    float ClampF(float value, float low, float high) { return value < low ? low : (value > high ? high : value); }

    bool SamePoint(SpriteCanvas::Point a, SpriteCanvas::Point b) {
        return a.x == b.x && a.y == b.y;
    }

    // キャンバス座標（ピクセル）→ 画面座標
    ImVec2 ToScreen(const ImVec2& origin, float zoom, int px, int py) {
        return ImVec2(origin.x + static_cast<float>(px) * zoom, origin.y + static_cast<float>(py) * zoom);
    }

    // パスの拡張子を小文字で返す（"." 込み。無ければ空文字）
    std::string GetExtensionLower(const std::string& path) {
        const size_t slash = path.find_last_of("/\\");
        const size_t dot = path.find_last_of('.');
        if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
            return std::string();
        }
        std::string ext = path.substr(dot);
        for (char& c : ext) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return ext;
    }

    // 拡張子（小文字）から保存形式のindexを返す（.png→0、.jpg/.jpeg→1、それ以外→-1）
    int FormatIndexFromExtension(const std::string& ext) {
        if (ext == ".png") {
            return 0;
        }
        if (ext == ".jpg" || ext == ".jpeg") {
            return 1;
        }
        return -1;
    }

    // std::stringをchar配列へコピーする（収まらない分は切り捨て。終端は必ず付く）
    void CopyToBuffer(char* buffer, size_t size, const std::string& text) {
        snprintf(buffer, size, "%s", text.c_str());
    }

    // パス欄の拡張子をformatIndexの形式に置き換える（画像の拡張子が無ければ付け足す）
    void ReplaceImageExtension(char* buffer, size_t size, int formatIndex) {
        std::string path = buffer;
        if (path.empty()) {
            return;
        }
        const std::string ext = GetExtensionLower(path);
        if (FormatIndexFromExtension(ext) >= 0) {
            path.erase(path.size() - ext.size());
        }
        path += kFormatExtensions[formatIndex];
        CopyToBuffer(buffer, size, path);
    }

    // UTF-8のパスにファイル（またはフォルダ）が存在するか
    bool FileExists(const std::string& utf8Path) {
        std::error_code ec;
        return std::filesystem::exists(std::filesystem::path(ConvertString(utf8Path)), ec);
    }

    // 区切り文字を "/" に揃える（TextureManagerのキャッシュキーと比較するため）
    std::string ToForwardSlashes(std::string path) {
        for (char& c : path) {
            if (c == '\\') {
                c = '/';
            }
        }
        return path;
    }

    // sRGB 符号化値（0〜1）をリニアへ戻す
    float SrgbToLinear(float c) {
        return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }

    // スウォッチ（Current・パレット）に渡す色。
    // ImGui は sRGB のバックバッファへ直接描くため、頂点色は GPU で sRGB 符号化されて実際より明るく表示される。
    // キャンバスのピクセル（sRGB テクスチャ）は符号化が相殺されてそのままの明るさなので、
    // スウォッチだけリニアへ戻した値を渡し、符号化後にキャンバスと同じ明るさになるようにする
    ImVec4 SwatchColor(const ImVec4& color) {
        return ImVec4(SrgbToLinear(color.x), SrgbToLinear(color.y), SrgbToLinear(color.z), color.w);
    }

} // namespace

SpriteEditor* SpriteEditor::GetInstance() {
    static SpriteEditor instance;
    return &instance;
}

void SpriteEditor::Initialize() {
    canvas_.Create(kInitialCanvasSize, kInitialCanvasSize);
    resizeWidth_ = canvas_.GetWidth();
    resizeHeight_ = canvas_.GetHeight();

    // キャンバスの内容を映す動的テクスチャ（ゲーム内のSpriteはこのハンドルで描く）
    textureHandle_ = TextureManager::GetInstance()->CreateDynamic(
        kTextureKey, static_cast<uint32_t>(canvas_.GetWidth()), static_cast<uint32_t>(canvas_.GetHeight()));
    uploadedWidth_ = canvas_.GetWidth();
    uploadedHeight_ = canvas_.GetHeight();
    UploadTextureIfChanged();
}

void SpriteEditor::DrawImGui() {
    if (open_) {
        ImGui::SetNextWindowPos(ImVec2(80.0f, 60.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(960.0f, 640.0f), ImGuiCond_FirstUseEver);
        // ホイールはキャンバス上で太さの変更に使うので、ウィンドウのスクロールには使わない
        if (ImGui::Begin("Sprite Editor", &open_, ImGuiWindowFlags_NoScrollWithMouse)) {
            // 左: ツールパネル（縦に長いので子ウィンドウでスクロールさせる）
            ImGui::BeginChild("ToolPanel", ImVec2(toolPanelWidth_, 0.0f), true);
            DrawToolPanel();
            ImGui::EndChild();

            // 仕切り（ドラッグでツールパネルの幅を変える。前後の間隔は空けず、仕切り自体の幅だけにする）
            ImGui::SameLine(0.0f, 0.0f);
            DrawSplitter();
            ImGui::SameLine(0.0f, 0.0f);

            // 右: キャンバス（残り全部。マウスとショートカットの処理もこの中で行う）
            DrawCanvasView();
        }
        ImGui::End();
    }

    // ウィンドウが閉じていても、未反映の内容はテクスチャへ送る（ゲーム側のSpriteが最新を映すように）
    UploadTextureIfChanged();
}

void SpriteEditor::DrawToolPanel() {
    // --- Color ---
    ImGui::SeparatorText("Color");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::ColorPicker4("##PenColor", penColor_,
        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview |
        ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex);
    // Current のスウォッチはキャンバスに塗ったときと同じ明るさで見せる（ピッカーの色面は ImGui 標準描画のため少し明るい）
    ImGui::ColorButton("##CurrentColor",
        SwatchColor(ImVec4(penColor_[0], penColor_[1], penColor_[2], penColor_[3])),
        ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(kCurrentColorButtonSize, kCurrentColorButtonSize));
    ImGui::SameLine();
    ImGui::Text("Current");

    // --- Palette ---
    ImGui::SeparatorText("Palette");
    if (ImGui::Button("+")) {
        // 現在のペン色を登録する（同じ値が既にあれば増やさない）
        const uint32_t color = GetPenColorU32();
        bool registered = false;
        for (uint32_t c : palette_) {
            if (c == color) {
                registered = true;
                break;
            }
        }
        if (!registered && palette_.size() < kMaxPaletteColors) {
            palette_.push_back(color);
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add current color");
    ImGui::TextDisabled("Left click: pick / Right click: remove");

    if (palette_.empty()) {
        ImGui::TextDisabled("No colors registered");
    } else {
        int removeIndex = -1;
        const int count = static_cast<int>(palette_.size());
        for (int i = 0; i < count; ++i) {
            ImGui::PushID(i);
            if (i > 0) {
                // 右端まで来たら折り返す
                ImGui::SameLine();
                if (ImGui::GetContentRegionAvail().x < kPaletteButtonSize) {
                    ImGui::NewLine();
                }
            }
            const ImVec4 color = ImGui::ColorConvertU32ToFloat4(palette_[i]);
            if (ImGui::ColorButton("##PaletteColor", SwatchColor(color), ImGuiColorEditFlags_AlphaPreviewHalf,
                    ImVec2(kPaletteButtonSize, kPaletteButtonSize))) {
                penColor_[0] = color.x;
                penColor_[1] = color.y;
                penColor_[2] = color.z;
                penColor_[3] = color.w;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                removeIndex = i;
            }
            ImGui::PopID();
        }
        if (removeIndex >= 0) {
            palette_.erase(palette_.begin() + removeIndex);
        }
    }

    // --- Brush ---
    ImGui::SeparatorText("Brush");
    // AlwaysClamp: Ctrl+クリックの数値入力でも 1〜100 の範囲に収める
    ImGui::SliderInt("Pen Size", &penSize_, SpriteCanvas::kMinBrushSize, SpriteCanvas::kMaxBrushSize,
        "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderInt("Eraser Size", &eraserSize_, SpriteCanvas::kMinBrushSize, SpriteCanvas::kMaxBrushSize,
        "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextDisabled("Ctrl+Wheel: pen / Shift+Wheel: eraser");

    // --- View ---
    ImGui::SeparatorText("View");
    ImGui::SliderInt("Zoom", &zoom_, kMinZoom, kMaxZoom, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::Checkbox("Grid", &showGrid_);
    ImGui::SameLine();
    if (ImGui::Button("Fit")) {
        // 表示領域に収まる最大の整数倍率にする（計算はDrawCanvasViewが同じフレーム内で行う）
        zoom_ = kZoomFitRequest;
    }
    ImGui::TextDisabled("Alt+Wheel: zoom / Middle drag: pan");

    // --- Canvas ---
    ImGui::SeparatorText("Canvas");
    ImGui::Text("Size: %d x %d", canvas_.GetWidth(), canvas_.GetHeight());
    ImGui::InputInt("Width", &resizeWidth_);
    ImGui::InputInt("Height", &resizeHeight_);
    resizeWidth_ = Clamp(resizeWidth_, SpriteCanvas::kMinSize, SpriteCanvas::kMaxSize);
    resizeHeight_ = Clamp(resizeHeight_, SpriteCanvas::kMinSize, SpriteCanvas::kMaxSize);
    if (ImGui::Button("Resize")) {
        // 中心基準でサイズを変える（1回の編集としてUndoできる）
        canvas_.Resize(resizeWidth_, resizeHeight_);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(keeps the center)");

    // --- Selection ---
    ImGui::SeparatorText("Selection");
    if (canvas_.HasSelection()) {
        const SpriteCanvas::Rect selection = canvas_.GetSelection();
        ImGui::Text("Selection: %d,%d  %d x %d", selection.x, selection.y, selection.width, selection.height);
        if (ImGui::Button("Confirm (Enter)")) {
            canvas_.CommitSelection();
        }
    } else {
        ImGui::TextDisabled("Ctrl+Drag to select");
    }

    // --- History ---
    ImGui::SeparatorText("History");
    ImGui::BeginDisabled(!canvas_.CanUndo());
    if (ImGui::Button("Undo (Ctrl+Z)")) {
        canvas_.Undo();
    }
    ImGui::EndDisabled();

    // --- File ---
    ImGui::SeparatorText("File");
    ImGui::SetNextItemWidth(-60.0f);
    if (ImGui::InputText("Path", pathBuffer_, sizeof(pathBuffer_))) {
        // 入力中のパスの拡張子が .png / .jpg なら、フォーマット選択もそれに合わせる（拡張子の付け足しは保存時に行う）
        const int formatIndex = FormatIndexFromExtension(GetExtensionLower(pathBuffer_));
        if (formatIndex >= 0) {
            formatIndex_ = formatIndex;
        }
    }
    ImGui::SetNextItemWidth(-60.0f);
    if (ImGui::Combo("Format", &formatIndex_, kFormatItems, IM_ARRAYSIZE(kFormatItems))) {
        // 形式を切り替えたら、パス欄の拡張子もそれに合わせる
        ReplaceImageExtension(pathBuffer_, sizeof(pathBuffer_), formatIndex_);
    }
    if (ImGui::Button("Save")) {
        // 拡張子を揃えてから存在を確認する（確認したパスと実際に保存するパスを一致させる）
        SyncPathAndFormat();
        if (pathBuffer_[0] == '\0') {
            statusMessage_ = "Path is empty";
        } else if (FileExists(pathBuffer_)) {
            ImGui::OpenPopup("Overwrite?");
        } else {
            SaveToFile();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        LoadFromFile();
    }

    // 上書き確認（モーダル。開いている間は他の操作を受け付けない）
    if (ImGui::BeginPopupModal("Overwrite?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s already exists. Overwrite?", pathBuffer_);
        if (ImGui::Button("Overwrite", ImVec2(120.0f, 0.0f))) {
            SaveToFile();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 直近の保存／読み込みの結果
    if (!statusMessage_.empty()) {
        ImGui::TextWrapped("%s", statusMessage_.c_str());
    }

    // --- Controls（操作一覧。既定は折りたたみ） ---
    if (ImGui::CollapsingHeader("Controls")) {
        ImGui::BulletText("Left drag: pen");
        ImGui::BulletText("Shift+Left click: fill");
        ImGui::BulletText("Right drag: eraser");
        ImGui::BulletText("Shift+Right click: transparent fill");
        ImGui::BulletText("Alt+Left click: eyedropper");
        ImGui::BulletText("Ctrl+Left drag: select");
        ImGui::BulletText("Left drag inside selection: move");
        ImGui::BulletText("Enter: confirm selection");
        ImGui::BulletText("Ctrl+Z: undo (%d)", SpriteCanvas::kMaxUndo);
        ImGui::BulletText("Ctrl+Wheel: pen size");
        ImGui::BulletText("Shift+Wheel: eraser size");
        ImGui::BulletText("Alt+Wheel: zoom");
        ImGui::BulletText("Middle drag: pan");
    }
}

void SpriteEditor::DrawSplitter() {
    // 仕切りは見えないボタン。押している間（IsItemActive）はカーソルが離れてもドラッグを続ける
    const float availHeight = ImGui::GetContentRegionAvail().y;
    ImGui::InvisibleButton("##Splitter", ImVec2(kSplitterWidth, availHeight > 1.0f ? availHeight : 1.0f));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    if (hovered || active) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (active) {
        // ツールパネルの幅を移動量ぶん変える。キャンバス表示に最低限の幅が残るように上限を決める
        const float contentWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
        const float maxToolPanelWidth = contentWidth - kSplitterWidth - kMinCanvasViewWidth;
        const float high = maxToolPanelWidth > kMinToolPanelWidth ? maxToolPanelWidth : kMinToolPanelWidth;
        toolPanelWidth_ = ClampF(toolPanelWidth_ + ImGui::GetIO().MouseDelta.x, kMinToolPanelWidth, high);
    }

    // 見た目: 中央に細い縦線（ホバー／ドラッグ中は明るく）
    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();
    const ImGuiCol colorId =
        active ? ImGuiCol_SeparatorActive : (hovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
    const float centerX = (rectMin.x + rectMax.x) * 0.5f;
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(centerX - 1.0f, rectMin.y), ImVec2(centerX + 1.0f, rectMax.y), ImGui::GetColorU32(colorId));
}

void SpriteEditor::DrawCanvasView() {
    // 横スクロールバーあり・ホイールでのスクロールなし（ホイールは太さの変更に使う）
    ImGui::BeginChild("CanvasView", ImVec2(0.0f, 0.0f), true,
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const int width = canvas_.GetWidth();
    const int height = canvas_.GetHeight();

    // Fitの要求があれば、表示領域に収まる最大の整数倍率にする（1未満なら1）
    if (zoom_ == kZoomFitRequest) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const int fitX = static_cast<int>(avail.x / static_cast<float>(width));
        const int fitY = static_cast<int>(avail.y / static_cast<float>(height));
        zoom_ = fitX < fitY ? fitX : fitY;
    }
    zoom_ = Clamp(zoom_, kMinZoom, kMaxZoom);

    // キャンバスの左上（画面座標。スクロール位置込み）。倍率には依存しない
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // Alt＋ホイールの拡大・縮小は、キャンバスを置く前に反映する
    // （この後のInvisibleButtonが新しい大きさを子ウィンドウへ申告し、スクロール範囲が今フレームで正しくなる）
    HandleZoomWheel(origin);

    const float zoom = static_cast<float>(zoom_);
    const ImVec2 canvasSize(static_cast<float>(width) * zoom, static_cast<float>(height) * zoom);
    const ImVec2 canvasMax(origin.x + canvasSize.x, origin.y + canvasSize.y);

    // キャンバス全体を1つのボタンにして左・右・中ボタンを受ける
    // （ボタンフラグを付けると、押している間はカーソルが外へ出てもアイテムがアクティブのまま保たれる）
    ImGui::InvisibleButton("##Canvas", canvasSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    // ホイールをImGuiのスクロールに取られないよう、ホバー中は所有権を取る
    if (hovered) {
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);
    }

    // カーソル位置 → キャンバス座標（範囲外の値もそのまま使う。クリップはSpriteCanvas側が行う）。
    // マウスが無いフレーム（ウィンドウ外など）は直前の位置を使い、変換の値が飛ばないようにする
    SpriteCanvas::Point pixel = lastPixel_;
    if (ImGui::IsMousePosValid()) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        pixel.x = static_cast<int>(std::floor((mouse.x - origin.x) / zoom));
        pixel.y = static_cast<int>(std::floor((mouse.y - origin.y) / zoom));
    }

    // --- 描画 ---
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 可視領域 = 子ウィンドウ ∩ キャンバス矩形（市松とグリッドはこの範囲だけ描いてコストを抑える）
    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const ImVec2 visibleMin(MaxF(origin.x, windowPos.x), MaxF(origin.y, windowPos.y));
    const ImVec2 visibleMax(
        MinF(canvasMax.x, windowPos.x + windowSize.x), MinF(canvasMax.y, windowPos.y + windowSize.y));
    const bool visible = visibleMin.x < visibleMax.x && visibleMin.y < visibleMax.y;

    drawList->PushClipRect(origin, canvasMax, true);

    // 市松模様（透明の下地）: 明色を敷いてから暗色のセルだけ重ねる
    if (visible) {
        drawList->AddRectFilled(visibleMin, visibleMax, kCheckerLight);
        const int colBegin = static_cast<int>(std::floor((visibleMin.x - origin.x) / kCheckerCellSize));
        const int colEnd = static_cast<int>(std::ceil((visibleMax.x - origin.x) / kCheckerCellSize));
        const int rowBegin = static_cast<int>(std::floor((visibleMin.y - origin.y) / kCheckerCellSize));
        const int rowEnd = static_cast<int>(std::ceil((visibleMax.y - origin.y) / kCheckerCellSize));
        for (int row = rowBegin; row < rowEnd; ++row) {
            for (int col = colBegin; col < colEnd; ++col) {
                if (((row + col) & 1) == 0) {
                    continue;
                }
                const float x0 = origin.x + static_cast<float>(col) * kCheckerCellSize;
                const float y0 = origin.y + static_cast<float>(row) * kCheckerCellSize;
                drawList->AddRectFilled(
                    ImVec2(x0, y0), ImVec2(x0 + kCheckerCellSize, y0 + kCheckerCellSize), kCheckerDark);
            }
        }
    }

    // キャンバス画像（点サンプリングで、拡大してもにじまない）
    ImGuiManager* imguiManager = ImGuiManager::GetInstance();
    imguiManager->PushPointSampling(drawList);
    drawList->AddImage(
        reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->GetSrvHandleGPU(textureHandle_).ptr),
        origin, canvasMax);
    imguiManager->PopPointSampling(drawList);

    // グリッド（拡大時のみ。可視範囲にあるピクセルの境界線だけ描く。外周はキャンバスの縁なので省く）
    if (visible && showGrid_ && zoom_ >= kGridMinZoom) {
        int xBegin = static_cast<int>(std::ceil((visibleMin.x - origin.x) / zoom));
        int xEnd = static_cast<int>(std::floor((visibleMax.x - origin.x) / zoom));
        xBegin = xBegin < 1 ? 1 : xBegin;
        xEnd = xEnd > width - 1 ? width - 1 : xEnd;
        for (int i = xBegin; i <= xEnd; ++i) {
            const float x = origin.x + static_cast<float>(i) * zoom;
            drawList->AddLine(ImVec2(x, visibleMin.y), ImVec2(x, visibleMax.y), kGridColor);
        }
        int yBegin = static_cast<int>(std::ceil((visibleMin.y - origin.y) / zoom));
        int yEnd = static_cast<int>(std::floor((visibleMax.y - origin.y) / zoom));
        yBegin = yBegin < 1 ? 1 : yBegin;
        yEnd = yEnd > height - 1 ? height - 1 : yEnd;
        for (int i = yBegin; i <= yEnd; ++i) {
            const float y = origin.y + static_cast<float>(i) * zoom;
            drawList->AddLine(ImVec2(visibleMin.x, y), ImVec2(visibleMax.x, y), kGridColor);
        }
    }

    // 選択枠（黒の外枠＋白の内枠。キャンバスからはみ出した分はクリップで切れる）
    if (canvas_.HasSelection() || canvas_.IsSelecting()) {
        const SpriteCanvas::Rect selection = canvas_.GetSelection();
        const ImVec2 selMin = ToScreen(origin, zoom, selection.x, selection.y);
        const ImVec2 selMax =
            ToScreen(origin, zoom, selection.x + selection.width, selection.y + selection.height);
        drawList->AddRect(selMin, selMax, kFrameBlack);
        drawList->AddRect(
            ImVec2(selMin.x + 1.0f, selMin.y + 1.0f), ImVec2(selMax.x - 1.0f, selMax.y - 1.0f), kFrameWhite);
    }

    // ブラシ枠（カーソル位置のスタンプの外接矩形。白の外枠＋黒の内枠）。
    // ペン／消しゴムのドラッグ中はカーソルが外へ出ても描く。範囲選択・移動中と、Alt（スポイト）を押している間は描かない
    const bool stroking = tool_ == Tool::kPen || tool_ == Tool::kEraser;
    if (stroking || (hovered && tool_ == Tool::kNone && !ImGui::GetIO().KeyAlt)) {
        const bool eraser =
            tool_ == Tool::kEraser || (tool_ == Tool::kNone && ImGui::IsMouseDown(ImGuiMouseButton_Right));
        const int size = eraser ? eraserSize_ : penSize_;
        const ImVec2 brushMin = ToScreen(origin, zoom, pixel.x - size / 2, pixel.y - size / 2);
        const ImVec2 brushMax(
            brushMin.x + static_cast<float>(size) * zoom, brushMin.y + static_cast<float>(size) * zoom);
        drawList->AddRect(brushMin, brushMax, kBrushFrameWhite);
        drawList->AddRect(
            ImVec2(brushMin.x + 1.0f, brushMin.y + 1.0f), ImVec2(brushMax.x - 1.0f, brushMax.y - 1.0f),
            kFrameBlack);
    }

    drawList->PopClipRect();

    // --- 入力 ---
    // 子ウィンドウの中（EndChildの前）で処理する: IsWindowFocusedの判定をこのウィンドウ階層で行うため
    HandleCanvasMouse(hovered, pixel);
    HandleShortcuts(hovered);

    ImGui::EndChild();
}

void SpriteEditor::HandleCanvasMouse(bool hovered, SpriteCanvas::Point pixel) {
    const ImGuiIO& io = ImGui::GetIO();
    // 修飾キーは左側のキーで判定する（Altはどちらでもよい）
    const bool ctrl = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);
    const bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift);
    const bool alt = io.KeyAlt;

    // --- 画面移動の開始（キャンバス表示のどこでもホイール押し込みで始められる） ---
    if (tool_ == Tool::kNone && (hovered || ImGui::IsWindowHovered()) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        tool_ = Tool::kPan;
    }

    // --- 操作の開始（キャンバス上でボタンが押されたとき） ---
    if (tool_ == Tool::kNone && hovered) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (alt) {
                // スポイト: クリックした位置の見た目どおりの色をペン色にする（透明なら変えない）
                if (canvas_.IsInside(pixel.x, pixel.y)) {
                    const uint32_t color = canvas_.GetCompositePixel(pixel.x, pixel.y);
                    if (((color >> IM_COL32_A_SHIFT) & 0xFF) != 0) {
                        const ImVec4 picked = ImGui::ColorConvertU32ToFloat4(color);
                        penColor_[0] = picked.x;
                        penColor_[1] = picked.y;
                        penColor_[2] = picked.z;
                        penColor_[3] = picked.w;
                    }
                }
            } else if (ctrl) {
                tool_ = Tool::kSelect;
                canvas_.BeginSelection(pixel);
            } else if (shift) {
                // 塗りつぶしは単発（ドラッグしない）
                canvas_.FloodFill(pixel, GetPenColorU32());
            } else if (canvas_.HasSelection() && canvas_.GetSelection().Contains(pixel.x, pixel.y)) {
                // 選択範囲の中から始めたドラッグは枠と中身の移動
                tool_ = Tool::kMove;
            } else {
                tool_ = Tool::kPen;
                canvas_.BeginStroke();
                canvas_.DrawLine(pixel, pixel, penSize_, GetPenColorU32());
            }
            lastPixel_ = pixel;
        } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (shift) {
                // 透過塗りつぶしは単発
                canvas_.FloodErase(pixel);
            } else {
                tool_ = Tool::kEraser;
                canvas_.BeginStroke();
                canvas_.EraseLine(pixel, pixel, eraserSize_);
            }
            lastPixel_ = pixel;
        }
    }

    // --- 操作の継続・終了（ボタンを離すまで。カーソルがキャンバスの外に出ても続ける） ---
    switch (tool_) {
    case Tool::kPen:
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!SamePoint(pixel, lastPixel_)) {
                // 前フレームの位置から線分補間して、速く動かしても途切れないようにする
                canvas_.DrawLine(lastPixel_, pixel, penSize_, GetPenColorU32());
                lastPixel_ = pixel;
            }
        } else {
            tool_ = Tool::kNone;
        }
        break;

    case Tool::kEraser:
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            if (!SamePoint(pixel, lastPixel_)) {
                canvas_.EraseLine(lastPixel_, pixel, eraserSize_);
                lastPixel_ = pixel;
            }
        } else {
            tool_ = Tool::kNone;
        }
        break;

    case Tool::kSelect:
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            canvas_.UpdateSelection(pixel);
        } else {
            canvas_.EndSelection();
            tool_ = Tool::kNone;
        }
        break;

    case Tool::kMove:
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const int dx = pixel.x - lastPixel_.x;
            const int dy = pixel.y - lastPixel_.y;
            if (dx != 0 || dy != 0) {
                canvas_.MoveSelection(dx, dy);
            }
            lastPixel_ = pixel;
        } else {
            tool_ = Tool::kNone;
        }
        break;

    case Tool::kPan:
        // ホイール押し込みドラッグ: マウスの移動量ぶんキャンバス表示をスクロールする
        // （この関数はキャンバスの子ウィンドウの中で呼ばれるので、SetScrollはその子ウィンドウに効く）。
        // 終了判定は「ボタンが押されていない」で行う（フォーカス喪失時にImGuiが離した扱いにしても固まらない）
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
                ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseDelta.x);
                ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
            }
        } else {
            tool_ = Tool::kNone;
        }
        break;

    case Tool::kNone:
    default:
        break;
    }
}

void SpriteEditor::HandleShortcuts(bool canvasHovered) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const bool typing = io.WantTextInput;  // パス欄などの入力中はキーを取らない
    // 上書き確認などのポップアップが開いている間はキーを取らない（ダイアログの裏で確定や Undo が走らないように）
    const bool popupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

    // エディタにフォーカスがある間は、Enter・Ctrl+Z などのキーがゲーム側（Engine::Input）へ届かないようにする
    // （io.WantCaptureKeyboard を次のフレームで立てる。Input::Update がそれを見てキーを無視する）
    if (focused) {
        ImGui::SetNextFrameWantCaptureKeyboard(true);
    }

    if (focused && !typing && !popupOpen && tool_ == Tool::kNone) {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            canvas_.Undo();
        }
        if (canvas_.HasSelection() &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))) {
            canvas_.CommitSelection();
        }
    }

    // ホイールで太さを変える（左Ctrl: ペン / 左Shift: 消しゴム。Alt＋ホイールの拡大縮小はHandleZoomWheelが担当。
    // 修飾キーなしでは何もしない）
    if (canvasHovered && io.MouseWheel != 0.0f && !io.KeyAlt) {
        const int delta = io.MouseWheel > 0.0f ? 1 : -1;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
            penSize_ = Clamp(penSize_ + delta, SpriteCanvas::kMinBrushSize, SpriteCanvas::kMaxBrushSize);
        } else if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
            eraserSize_ = Clamp(eraserSize_ + delta, SpriteCanvas::kMinBrushSize, SpriteCanvas::kMaxBrushSize);
        }
    }
}

void SpriteEditor::HandleZoomWheel(const ImVec2& origin) {
    const ImGuiIO& io = ImGui::GetIO();
    // キャンバスの子ウィンドウ上で Alt＋ホイールのときだけ（Ctrl／Shift＋ホイールは太さの変更）
    if (!ImGui::IsWindowHovered() || !io.KeyAlt || io.MouseWheel == 0.0f || !ImGui::IsMousePosValid()) {
        return;
    }
    const int delta = io.MouseWheel > 0.0f ? 1 : -1;
    const int newZoom = Clamp(zoom_ + delta, kMinZoom, kMaxZoom);
    if (newZoom == zoom_) {
        return;
    }

    // カーソルの下のキャンバス座標（ピクセル。小数あり）が倍率変更後も同じ画面位置に来るよう、
    // 位置のずれ（座標 × 倍率の差）だけスクロールを進める（スクロールできる範囲はImGuiがクランプする）
    const float oldZoom = static_cast<float>(zoom_);
    const float canvasX = (io.MousePos.x - origin.x) / oldZoom;
    const float canvasY = (io.MousePos.y - origin.y) / oldZoom;
    const float zoomDelta = static_cast<float>(newZoom) - oldZoom;
    ImGui::SetScrollX(ImGui::GetScrollX() + canvasX * zoomDelta);
    ImGui::SetScrollY(ImGui::GetScrollY() + canvasY * zoomDelta);
    zoom_ = newZoom;
}

void SpriteEditor::SaveToFile() {
    SyncPathAndFormat();
    const std::string path = pathBuffer_;
    const SpriteCanvas::ImageFormat format =
        (formatIndex_ == 1) ? SpriteCanvas::ImageFormat::kJpg : SpriteCanvas::ImageFormat::kPng;

    std::string error;
    if (!canvas_.Save(path, format, error)) {
        statusMessage_ = error;
        return;
    }
    statusMessage_ = "Saved: " + path + " (" + std::to_string(canvas_.GetWidth()) + " x " +
                     std::to_string(canvas_.GetHeight()) + ")";

    // ゲーム側がこのファイルをLoad済みなら読み直して、そのSpriteにも反映する（区切り文字の違いも吸収する）
    TextureManager* textureManager = TextureManager::GetInstance();
    const std::string normalized = ToForwardSlashes(path);
    if (textureManager->IsLoaded(path)) {
        textureManager->Reload(path);
    } else if (normalized != path && textureManager->IsLoaded(normalized)) {
        textureManager->Reload(normalized);
    }
}

void SpriteEditor::LoadFromFile() {
    const std::string path = pathBuffer_;
    if (path.empty()) {
        statusMessage_ = "Path is empty";
        return;
    }

    std::string error;
    if (!canvas_.Load(path, error)) {
        statusMessage_ = error;
        return;
    }
    // サイズ欄を読み込んだ画像に合わせる。拡張子が分かればフォーマット選択も合わせる
    resizeWidth_ = canvas_.GetWidth();
    resizeHeight_ = canvas_.GetHeight();
    const int formatIndex = FormatIndexFromExtension(GetExtensionLower(path));
    if (formatIndex >= 0) {
        formatIndex_ = formatIndex;
    }
    statusMessage_ = "Loaded: " + path + " (" + std::to_string(canvas_.GetWidth()) + " x " +
                     std::to_string(canvas_.GetHeight()) + ")";
}

void SpriteEditor::SyncPathAndFormat() {
    std::string path = pathBuffer_;
    if (path.empty()) {
        return;
    }
    // 拡張子が .png / .jpg / .jpeg ならフォーマットをそれに合わせ、無ければ現在のフォーマットの拡張子を付け足す
    const int formatIndex = FormatIndexFromExtension(GetExtensionLower(path));
    if (formatIndex >= 0) {
        formatIndex_ = formatIndex;
        return;
    }
    path += kFormatExtensions[formatIndex_];
    CopyToBuffer(pathBuffer_, sizeof(pathBuffer_), path);
}

void SpriteEditor::UploadTextureIfChanged() {
    if (canvas_.GetRevision() == uploadedRevision_) {
        return;
    }

    TextureManager* textureManager = TextureManager::GetInstance();
    const int width = canvas_.GetWidth();
    const int height = canvas_.GetHeight();
    if (width != uploadedWidth_ || height != uploadedHeight_) {
        // サイズが変わった（リサイズ・読み込み・Undo）ときだけ作り直す（GPU完了待ちが入る）
        textureManager->ResizeDynamic(textureHandle_, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        uploadedWidth_ = width;
        uploadedHeight_ = height;
    }
    // 合成結果（移動中の選択範囲も重ねた見た目）を送るので、ゲーム側にも編集途中が見える
    textureManager->UpdateDynamic(textureHandle_, canvas_.GetComposite().data());
    uploadedRevision_ = canvas_.GetRevision();
}

uint32_t SpriteEditor::GetPenColorU32() const {
    // 各成分を0〜1にクランプして四捨五入で8bitへ
    auto toByte = [](float v) {
        v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        return static_cast<int>(v * 255.0f + 0.5f);
    };
    return IM_COL32(toByte(penColor_[0]), toByte(penColor_[1]), toByte(penColor_[2]), toByte(penColor_[3]));
}

} // namespace Engine

#endif  // USE_IMGUI
