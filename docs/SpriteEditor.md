# スプライトエディタ（Sprite Editor）

ゲームを動かしたまま、マウスで 2D スプライトを描き、その場でゲーム画面に出して確かめ、気に入ったら PNG / JPG に保存できるエンジン内ツールです。
別のお絵かきアプリで作ってファイルをコピーして……という手間を挟まずに、エンジンの中だけでスプライト作りが完結します。

- 使えるビルド: **Debug / Development**（ImGui があるビルド。`USE_IMGUI`）。Release にはエディタのコードごと入りません
- 場所: `Engine/Tools/SpriteEditor.*`（ImGui ウィンドウ・入力・テクスチャ反映）、`Engine/Tools/SpriteCanvas.*`（ピクセル配列と編集処理）

## 開き方

Sandbox では起動時に開いています。閉じたときは「Display」ウィンドウの **Sprite Editor** チェック、または「2D Objects」→「Sprite Editor Preview」の **Open Sprite Editor** ボタンで開き直せます。
ゲーム側のコードから開くときは次の 1 行です。

```cpp
Engine::SpriteEditor::GetInstance()->SetOpen(true);
```

ウィンドウは他の ImGui ウィンドウと同じようにドッキングや移動ができます。

## 画面の見かた

左のパネルが道具、右がキャンバスです。

| 左パネル | 内容 |
|---|---|
| Color | ペンの色（カラーピッカー。アルファも指定できる） |
| Palette | 色の登録。**+** で今のペン色を登録、**左クリック**で選択、**右クリック**で解除 |
| Brush | ペン / 消しゴムの太さ（1〜100 px） |
| View | 表示倍率（Zoom。Alt + ホイールでも変更可）、グリッド表示、Fit（領域に収まる倍率にする） |
| Canvas | キャンバスのサイズ。Width / Height に数値を入れて **Resize**（中心を基準に広げる／切り詰める） |
| Selection | 範囲選択の状態と **Confirm**（Enter と同じ） |
| History | **Undo**（Ctrl+Z と同じ。直近 5 回） |
| File | 保存先のパス、形式（PNG / JPG）、**Save** / **Load** |
| Controls | 操作一覧（英語） |

キャンバスの透明な所は市松模様で表示されます。
左パネルとキャンバスの間の仕切りはドラッグで動かせます（キャンバスを広く使いたいときは左へ寄せる。幅は起動時に 280px に戻ります）。

## 操作一覧

| 操作 | 動き |
|---|---|
| 左ドラッグ | ペンで塗る |
| 左Shift + 左クリック | 塗りつぶし（クリックした所と同じ色の連続した領域を、ペンの色で塗る） |
| 右ドラッグ | 消しゴム（透明にする） |
| 左Shift + 右クリック | 透過塗りつぶし（同じ色の連続した領域を透明にする） |
| Alt + 左クリック | スポイト（クリックした所の色をペンの色にする。透明な所では変わらない） |
| 左Ctrl + ホイール回転 | ペンの太さを変える（1〜100 px） |
| 左Shift + ホイール回転 | 消しゴムの太さを変える（1〜100 px） |
| Alt + ホイール回転 | キャンバス表示の拡大・縮小（カーソルの下のピクセルを基準に 1〜32 倍） |
| ホイール押し込みドラッグ | キャンバス表示の移動（表示領域に収まっているときは動かない） |
| 左Ctrl + 左ドラッグ | 範囲選択（矩形） |
| 選択範囲の中で左ドラッグ | 選択範囲を枠ごと動かす |
| Enter | 選択範囲を確定する（その位置に焼き込む） |
| 左Ctrl + Z | 元に戻す（直近 5 回。塗り・塗りつぶし・消し・範囲の移動・リサイズ・読み込みがそれぞれ 1 回ぶん） |

範囲選択中のルール:

- 選択範囲の**外**ではペン・消しゴム・塗りつぶしが普通に使えます。選択範囲の**中**は保護され、変わりません（中で左ドラッグすると移動になるため）
- 動かさずに Enter で確定した場合は何も変わっていないので、Undo の回数には数えません
- キャンバスの外へはみ出した部分は、確定時に捨てられます
- 新しく範囲選択を始めると、前の選択範囲は自動で確定されます

キーボードのショートカットは、Sprite Editor のウィンドウ（キャンバス含む）にフォーカスがあるときに効きます。
Sprite Editor を操作している間は、同じキーやマウス操作がゲーム側には届きません（ゲーム画面をクリックするとゲーム側に戻ります）。

## 保存と読み込み

- **Save**: File のパス（実行ディレクトリ＝`Sandbox/` や `Game/` からの相対パス）に保存します。既定は `resources/sprite.png` です
  - `resources/sprites/player.png` のようにサブフォルダを書けば、無いフォルダは自動で作られます。後でフォルダ構成を分けるときは、パスを変えるだけで済みます
  - 形式は Format で選びます（既定 PNG）。パスの拡張子を `.png` / `.jpg` にすると、Format もそれに合わせて切り替わります
  - **PNG** は塗っていない所を透過として保存します。**JPG** は塗っていない所を白にして保存します（JPG に透過はありません）
  - 画像サイズ＝キャンバスサイズです
  - 同名のファイルがあるときは上書き確認が出ます
  - 保存したファイルをゲーム側で既に読み込んでいる（`sprite.Initialize("resources/sprite.png", …)` などで使っている）場合は、その場で差し替わります（再起動は不要）
- **Load**: パスの画像を読み込み、キャンバスを置き換えます（サイズも画像に合わせます。1024×1024 より大きい画像は読み込めません）。読み込みも Undo で戻せます

## ゲームで使う

エディタのキャンバスはテクスチャとして公開されているので、普通の `Sprite` にハンドルを渡すだけで描いた内容がそのまま出ます（描いている途中もリアルタイムで反映されます）。

```cpp
#include "Engine/Tools/SpriteEditor.h"

// OnInitialize（Debug / Development だけで有効にする）
#ifdef USE_IMGUI
    Engine::SpriteEditor* editor = Engine::SpriteEditor::GetInstance();
    editorSprite_.Initialize(
        editor->GetTextureHandle(),
        { float(editor->GetCanvasWidth()), float(editor->GetCanvasHeight()) });
    editor->SetOpen(true);
#endif

// OnUpdate（キャンバスをリサイズしたらスプライトのサイズも合わせる。変わったときだけ SetSize する）
#ifdef USE_IMGUI
    const Engine::SpriteEditor* editor = Engine::SpriteEditor::GetInstance();
    const Engine::Vector2 canvasSize{ float(editor->GetCanvasWidth()), float(editor->GetCanvasHeight()) };
    if (editorSprite_.GetSize().x != canvasSize.x || editorSprite_.GetSize().y != canvasSize.y) {
        editorSprite_.SetSize(canvasSize);
    }
    editorSprite_.Update();
#endif

// OnDraw
#ifdef USE_IMGUI
    editorSprite_.Draw();
#endif
```

`SpriteEditor` は `USE_IMGUI` のあるビルドにしか無いので、使う場所は `#ifdef USE_IMGUI` で囲みます。
保存したあとは、普通どおり `sprite.Initialize("resources/sprite.png", { 64.0f, 64.0f });` で読み込めます（Release でも使えます）。

Sandbox では `GameScene`（起動時のシーン）に「Sprite Editor Preview」としてこの仕組みが入っています。位置・拡大率は「2D Objects」ウィンドウで調整できます。

## 仕組みと注意

- キャンバスは CPU 側の RGBA8 配列で、変更があったフレームだけ GPU の**動的テクスチャ**（`TextureManager::CreateDynamic` / `UpdateDynamic`）へ転送します。転送は `Framework` が毎フレーム `TextureManager::BeginFrame` で行います
- テクスチャはファイルから読むものと同じ sRGB フォーマットなので、「エディタの見た目」「ゲームでの見た目」「保存して読み直した見た目」は一致します
- エディタ内の拡大表示は点サンプリング（`ImGuiManager::PushPointSampling`）で描くので、ドットがにじみません。ゲーム内の `Sprite` はバイリニア補間のため、等倍以外で出すと少しぼやけます
- ImGui の標準フォントは ASCII のみのため、エディタ内の表示は英語です
- ImGui は sRGB のバックバッファへ直接描いているため、ImGui の色（カラーピッカーの色面など）は実際より少し明るく表示されます。Current とパレットのスウォッチは補正してあり、キャンバスに塗った色と同じ明るさで見えます。ピッカーの数値（R/G/B・16進）は正確です
- キャンバスの初期サイズは 64×64、上限は 1024×1024 です
- 色はカラーピッカーで選ぶ単色のみで、グラデーション機能はありません
