# エンジン機能 棚卸し（AL4_develop / Engine.lib）

対象: `C:\Users\s-dai\source\repos\DirectXGame`（ブランチ AL4_develop、2026-10-07 時点）。
情報源の順: `Engine/Docs/EngineReference.html`（2026-10-07 更新分まで）→ `Engine/` の公開ヘッダ（行番号はこのブランチの実ファイル）→ `Game/`・`docs/`。
「無し」は Engine/ 配下を grep して確認した断定。推測が混じるところは「要確認」と書く。

凡例: 〔シングルトン〕= `GetInstance()` で取得 / 〔static〕= インスタンス不要 / 〔USE_IMGUI〕= Debug・Development のみ（Release には存在しない）/ 〔Release 除外〕= 使う側で `#ifndef NDEBUG` で囲む / 〔エンジンが呼ぶ〕= ゲームからは呼ばない。
名前空間はすべて `Engine`（`using namespace Engine;` 可）。インクルードはリポジトリ直下基準（`#include "Engine/…"`、`#include "Game/…"`）。

---

## 1. 骨組み

### エントリポイントとアプリクラス
- **WinMain** — `MyGame game; game.Run();` だけ。引数は未使用 — Game/main.cpp:6
- **Engine::Framework**〔継承して使う〕— アプリ全体の骨組み。ゲームは `MyGame : public Engine::Framework` を 1 つ作る — Engine/Core/Framework.h:20
- **Framework::Run()** — 初期化 → メインループ → 終了処理。ウィンドウが閉じるまで戻らない — Engine/Core/Framework.h:26
- **Framework::Initialize()**〔protected virtual〕— エンジン各機能の初期化。順: COM → クラッシュダンプ → ログ → Time → WinApp → DirectXCore → ShaderCompiler → DescriptorHeap → PipelineManager → DebugDraw → TextureManager → StereoRenderer → Audio → Input → ImGui → SpriteEditor。オーバーライド時は `SetWindowTitle → Framework::Initialize() → ChangeScene<最初>()` の順 — Engine/Core/Framework.h:32（実装 Engine/Core/Framework.cpp:160-204）
- **Framework::Finalize()**〔protected virtual〕— シーン破棄 → 各機能を逆順で解放 → DXGI リークチェック。自前リソースを解放してから最後に基底を呼ぶ — Engine/Core/Framework.h:35
- **Framework::Update()**〔protected virtual〕— 既定は `SceneManager::Update()`。全シーン共通処理を足すならオーバーライドして最後に基底を呼ぶ — Engine/Core/Framework.h:38
- **Framework::Draw(commandList, viewIndex)**〔protected virtual〕— 既定は現在シーンの描画。立体視 OFF なら viewIndex=0 で 1 回 — Engine/Core/Framework.h:42
- **Framework::PreDraw(commandList)**〔protected virtual〕— 画面クリア後・シーン描画前のフック（既定は空） — Engine/Core/Framework.h:45
- **Framework::DrawImGui()**〔protected virtual, USE_IMGUI〕— 既定はシーンの OnDrawImGui ＋ SpriteEditor のウィンドウ。全シーン共通 UI を出すならオーバーライドして基底も呼ぶ — Engine/Core/Framework.h:49
- **Framework::SetWindowTitle(const std::wstring&)** — タイトル設定（Initialize 前に呼べば最初からそのタイトル） — Engine/Core/Framework.h:53
- **Framework::ChangeScene<TScene>()** — 次シーンの予約（SceneManager に委譲） — Engine/Core/Framework.h:56

### 1 フレームの流れ（Framework::Run のメインループ。Engine/Core/Framework.cpp:46-152）
1. `WinApp::ProcessMessage()` でたまったメッセージを全部処理（× ボタンでループ終了） — Framework.cpp:46
2. ウィンドウサイズが変わっていればスワップチェーン・深度バッファ等を作り直す — :49-54
3. `SceneManager::ApplySceneChange()`（予約シーンへ切替: GPU 完了待ち → 旧シーン破棄 → 新シーンの `Initialize`→`OnInitialize`） — :57
4. 〔USE_IMGUI〕ImGui BeginFrame → `DrawImGui()`（シーンの **OnDrawImGui** → SpriteEditor）→ Render — :59-63
5. `Input::Update()`（この後の OnUpdate が今フレームの入力を見る。**OnDrawImGui の中で読む Input は前フレームの値**） — :66
6. F11 でボーダレス全画面トグル（エンジン予約キー） — :69-71
7. ゲームの描画先矩形をシーンへ渡す（ImGui ビルドはドッキングで空いた中央領域、Release はウィンドウ全体） — :75-83
8. `Update()` → `BaseScene::Update`: 描画先サイズを RenderContext へ → **OnUpdate** → 射影・ビュー行列を確定 → 視錐台 → 光源 CB 書込 → 立体視カメラ更新 — Engine/Scene/BaseScene.cpp:26-50
9. `DirectXCore::BeginFrame()`（クリア色 RGB(0.1,0.25,0.5) 固定）→ `TextureManager::BeginFrame`（動的テクスチャ転送）→ `PreDraw` — Framework.cpp:87-93
10. `Draw()` → `BaseScene::Draw`: ルートシグネチャ・ビュー射影・光源・標準 PSO 設定 → **OnDraw** → `DebugDraw::Render`（線を最後に深度テスト無しで） — Engine/Scene/BaseScene.cpp:52-78
11. 〔USE_IMGUI〕ImGui を画面全体に描画 — Framework.cpp:134-143
12. `EndFrame()`（Present、垂直同期あり）→ `DebugDraw::EndFrame()`（線を消す）→ `Time::WaitForNextFrame()`（1/60 秒まで待つ） — :145-151

### ゲームループ・シーン管理（エンジン側に **有り**）
- **Engine::SceneManager**〔シングルトン〕— 現在シーンを 1 つだけ保持。切替は予約制（次フレーム頭）。シーンの登録は不要 — Engine/Scene/SceneManager.h:19
- **SceneManager::ChangeScene(std::unique_ptr<BaseScene>)** — 作成済みシーンを予約。**コンストラクタ引数を渡したいとき**は `ChangeScene(std::make_unique<StageScene>(2))` — Engine/Scene/SceneManager.h:25
- **SceneManager::ChangeScene<TScene>()** — 型で予約（内部で `make_unique<TScene>()`）。同フレームに複数回呼んだら最後が有効 — Engine/Scene/SceneManager.h:28
- **SceneManager::GetCurrentScene()** — 現在シーン（最初の切替前は nullptr） — Engine/Scene/SceneManager.h:32
- **Engine::BaseScene**〔継承して使う〕— シーン基底。切替のたびに新しく作られ前のシーンは破棄される（メンバーは毎回初期状態） — Engine/Scene/BaseScene.h:27
- **BaseScene::OnInitialize() / OnUpdate() / OnDraw()**〔pure virtual〕— 開始時 1 回 / 毎フレームの処理＋各 Update() / 各 Draw() だけ（立体視では複数回呼ばれるので状態を変えない） — Engine/Scene/BaseScene.h:69-75
- **BaseScene::OnDrawImGui()**〔virtual, USE_IMGUI〕— 開発用 UI。OnUpdate より先に呼ばれる — Engine/Scene/BaseScene.h:79
- **BaseScene::CalcViewMatrix()**〔virtual〕— 描画用ビュー行列（既定 `camera_.GetViewMatrix()`）。デバッグカメラ差替用。**射影行列は差し替え不可（非 virtual）** — Engine/Scene/BaseScene.h:84
- **BaseScene::ChangeScene<TScene>()** — シーン内から次シーンを予約。呼んだ後も同フレームの OnUpdate/OnDraw は最後まで走る（止めたければ `return`） — Engine/Scene/BaseScene.h:89
- **BaseScene::GetRenderAreaX() / GetRenderAreaY() / GetRenderAreaWidth() / GetRenderAreaHeight()** — ゲーム描画先矩形（クライアント座標 px）。マウス座標との対応付けに必須 — Engine/Scene/BaseScene.h:93-96
- **BaseScene::GetAspectRatio()** — 描画先の幅÷高さ — Engine/Scene/BaseScene.h:99
- **BaseScene::GetViewMatrix() / GetProjectionMatrix()** — 直近 Update で確定した行列（OnUpdate 内で読むと前フレームの値、OnDraw 内なら今フレーム） — Engine/Scene/BaseScene.h:102-103
- **BaseScene::camera_ / directionalLight_ / pointLights_ / stereoCamera_**〔protected メンバー〕— シーンのカメラ・平行光源・点光源 4 個・立体視カメラ — Engine/Scene/BaseScene.h:106-115
- 無いもの: シーンのスタック（ポーズ画面の重ね表示）、フェード等の遷移演出、シーン間でデータを渡す仕組み（コンストラクタ引数か、ゲーム側の static / MyGame のメンバーで持つ）、シーンの再利用（毎回作り直し）、ゲームオブジェクトの一括管理。

### ゲーム側に新しいクラスを追加する手順（docs に明文化は無し。Game.vcxproj から読み取り）
- `Game/Game.vcxproj` の ItemGroup に `<ClCompile Include="Object\Foo.cpp" />` と `<ClInclude Include="Object\Foo.h" />` を追加する — Game/Game.vcxproj:65-70（cpp）, :73-77（h）
- `Game/Game.vcxproj.filters` にも同じ項目を `<Filter>Object</Filter>`（または `Scene`）付きで追加する。既存フィルタは **Object / Scene** の 2 つ — Game/Game.vcxproj.filters:4-7（Enemy.cpp/.h は Filter 無しでルートに登録されている :23, :36 — 既存の不揃い）
- Visual Studio のソリューションエクスプローラーでフィルタを右クリック →「追加 → 新しい項目」で作れば両ファイルとも自動更新される（新しいフィルタも VS の「新しいフィルター」で可）。
- インクルードはリポジトリ直下基準なので `#include "Game/Object/Foo.h"`（Engine.props の AdditionalIncludeDirectories=$(RepoRoot)） — Engine/Engine.props:32
- 実行時カレントディレクトリは `Game/`（LocalDebuggerWorkingDirectory=$(ProjectDir)） — Game/Game.vcxproj:61
- Initialize 済みの Object3D / Sprite はコピーして両方使えない（定数バッファを共有）。配列で持つなら `std::vector` を reserve してから要素ごとに Initialize するか、`std::unique_ptr` で持つ（EngineReference「つまずきやすい点」）。

---

## 2. 入力（Engine/Input/Input.h〔シングルトン〕。`DIK_*` は Input.h を include すれば使える）

### キーボード
- **Input::GetInstance()** — Engine/Input/Input.h:50
- **Input::IsPress(uint8_t keyNumber)** — 押している間ずっと true（`DIK_W` など） — Engine/Input/Input.h:66
- **Input::IsTrigger(uint8_t keyNumber)** — 押した瞬間のフレームだけ true — Engine/Input/Input.h:72
- **Input::IsRelease(uint8_t keyNumber)** — 離した瞬間のフレームだけ true — Engine/Input/Input.h:78

### マウス（座標・左右中ボタン・ホイール すべて **有り**）
- **enum MouseButton { kMouseLeft=0, kMouseRight=1, kMouseMiddle=2 }** — Engine/Input/Input.h:16
- **Input::IsMousePress(int button) / IsMouseTrigger(int) / IsMouseRelease(int)** — 押下中 / 押した瞬間 / 離した瞬間。0〜7 以外は false — Engine/Input/Input.h:83-89
- **Input::GetMouseMove()** — 今フレームの相対移動量 `Vector2`（右 +x、下 +y） — Engine/Input/Input.h:92
- **Input::GetWheel()** — 今フレームのホイール回転量 `float`（奥 +、手前 −。1 目盛り ±120） — Engine/Input/Input.h:95
- **Input::GetMousePosition()** — カーソルのクライアント座標 `Vector2`（左上原点・px）。Debug/Development では ImGui の分ずれるので `GetRenderAreaX/Y()` を引く。さらにスプライトの 1280×720 基準座標へ直すには `(x−areaX)×1280/areaWidth` の換算が必要（**その関数は無し**） — Engine/Input/Input.h:98
- 無いもの: カーソルの表示/非表示・ロック・移動（Win32 の `ShowCursor`/`ClipCursor`/`SetCursorPos` を `WinApp::GetHwnd()` と組み合わせて自前で）、文字入力、DIK→文字変換、キーリピート、キーコンフィグ。

### ゲームパッド（XInput・最大 4 人。playerIndex 省略で 1P）**有り**
- **enum PadButton { kPadUp, kPadDown, kPadLeft, kPadRight, kPadStart, kPadBack, kPadLeftThumb, kPadRightThumb, kPadLeftShoulder, kPadRightShoulder, kPadA, kPadB, kPadX, kPadY }** — 値は XINPUT_GAMEPAD_* のビット。LT/RT はボタンではなくトリガー関数で読む — Engine/Input/Input.h:23-38
- **Input::IsPadConnected(int playerIndex=0)** — Engine/Input/Input.h:105
- **Input::IsPadPress / IsPadTrigger / IsPadRelease(int button, int playerIndex=0)** — Engine/Input/Input.h:108-114
- **Input::GetLeftTrigger / GetRightTrigger(int playerIndex=0)** — 0〜1、デッドゾーン処理済み — Engine/Input/Input.h:117-118
- **Input::GetLeftStick / GetRightStick(int playerIndex=0)** — 各成分 −1〜1、**上が +y**（マウスは下が +y） — Engine/Input/Input.h:121-122
- **Input::SetVibration(left, right, playerIndex=0) / SetVibrationForTime(left, right, seconds, playerIndex=0) / StopVibration(playerIndex=0)** — 振動（即時 / 秒指定で自動停止 / 停止） — Engine/Input/Input.h:131-140

### 取得条件・ImGui 操作中の扱い
- キーボード・マウスは DirectInput の前面モード（DISCL_FOREGROUND）。ウィンドウが背面だと何も押されていない扱い。パッドは背面でも取れる — Engine/Input/Input.cpp:46,58
- 〔USE_IMGUI〕`ImGuiIO::WantCaptureKeyboard` の間は全キー 0、`WantCaptureMouse` の間はマウスボタンとホイールが 0。**カーソル位置（GetMousePosition）・GetMouseMove・パッドはそのまま**。ゲーム画面をクリックすると戻る — Engine/Input/Input.cpp:100-115
- F11 はエンジンが全画面切替に使う（Framework.cpp:69）。DebugCamera を使うシーンでは Enter も ON/OFF に取られる。
- OnDrawImGui の中で Input を読むと前フレームの値（Input::Update が DrawImGui の後にある）。

---

## 3. 2D

### Sprite（Engine/Rendering/Sprite.h）
- **Sprite::Initialize(const std::string& textureFilePath, const Vector2& size)** — 画像パスと表示サイズ（1280×720 基準 px。画像解像度と無関係にこの大きさに引き伸ばす） — Engine/Rendering/Sprite.h:33
- **Sprite::Initialize(uint32_t textureHandle, const Vector2& size)** — テクスチャハンドル版（SpriteEditor / TextureManager のハンドル） — Engine/Rendering/Sprite.h:29
- **Sprite::Update()** — 行列・正射影・マテリアルを CB へ書き込み、画面外判定。毎フレーム Draw 前に必須 — Engine/Rendering/Sprite.h:37
- **Sprite::Draw() const** — OnDraw の中で **3D の後に** 呼ぶ（深度 0 で書くので常に手前。後から Draw したものが上に重なる＝描画順は呼び順） — Engine/Rendering/Sprite.h:42
- **Sprite::GetTransform()** — `translate.x/y`=左上の位置 px、`scale.x/y`=倍率、`rotate.z`=回転 rad。**回転・拡縮の軸は左上の角で固定**（アンカー設定は無し。Engine/Rendering/Sprite.cpp:22-27,78） — Engine/Rendering/Sprite.h:44
- **Sprite::SetSize(const Vector2&) / GetSize()** — 表示サイズ変更（4 頂点の書換えだけ。再 Initialize 不要）。画像の実寸に合わせるなら `TextureManager::GetWidth/GetHeight` と組み合わせる — Engine/Rendering/Sprite.h:47-48
- **Sprite::GetUVTransform()** — UV 変換（scale・rotate.z・translate）。連番アニメは `scale={1/列数,1/行数,1}`、`translate`=コマ左上の UV — Engine/Rendering/Sprite.h:51
- **Sprite::GetMaterial()** — 色（`color.w` で不透明度）。ライティングは常に無効 — Engine/Rendering/Sprite.h:54
- **Sprite::SetColor(const Vector4&) / SetColor(uint32_t 0xRRGGBBAA)** — テクスチャ色に乗算 — Engine/Rendering/Sprite.h:57-58
- **Sprite::GetMappedVertices()** — 4 頂点の直接編集（0:左下 1:左上 2:右下 3:右上） — Engine/Rendering/Sprite.h:61
- **Sprite::SetTextureHandle(uint32_t)** — テクスチャ差替（**ゲッター GetTextureHandle は無し**） — Engine/Rendering/Sprite.h:63
- **Sprite::GetVisibility()** — 直近 Update の画面内判定（FrustumVisibility） — Engine/Rendering/Sprite.h:65
- 挙動: 実描画先が 1280×720 と違うとき、サイズは縦横の小さい方の倍率で等比、位置は縦横それぞれ比例（Sprite.cpp:66-77）。テクスチャのアルファ < 0.8 のピクセルは描かれない（Shaders/Object3d.PS.hlsl:73-75）。全体を薄くするなら SetColor のアルファで。
- 無いもの: アンカー / ピボット、左右・上下反転フラグ（UV 変換で `scale.x=-1, translate.x=1` にすれば鏡像になる見込み・**要確認**。Transform の scale を負にすると裏面カリングで消えるので不可）、描画順 / レイヤー指定（呼び順のみ）、文字・数字描画、9 スライス、スプライトシート再生ヘルパー、バッチ描画。

### SpriteEditor / SpriteCanvas（ゲームで何に使えるか）
- **Engine::SpriteEditor**〔シングルトン, USE_IMGUI〕— ImGui の「Sprite Editor」ウィンドウで絵を描き、その場で Sprite に映し、PNG/JPG に保存。ヘッダごと `#ifdef USE_IMGUI` — Engine/Tools/SpriteEditor.h:25
- **SpriteEditor::GetInstance()** — Engine/Tools/SpriteEditor.h:28
- **SpriteEditor::SetOpen(bool) / IsOpen()** — ウィンドウの開閉（既定は閉。× で閉じても IsOpen が false になる） — Engine/Tools/SpriteEditor.h:39-40
- **SpriteEditor::GetTextureHandle()** — キャンバスの動的テクスチャハンドル（Framework::Initialize 後は不変）。`Sprite::Initialize(handle, size)` / `SetTextureHandle` に渡すと描いた内容がリアルタイム反映 — Engine/Tools/SpriteEditor.h:43
- **SpriteEditor::GetCanvasWidth() / GetCanvasHeight()** — キャンバス px（初期 64×64、最大 1024×1024。Resize/Load で変わるので毎フレーム比べて `SetSize`） — Engine/Tools/SpriteEditor.h:46-47
- 用途: 仮絵（プレイヤー・ブーメラン・UI）をゲームを止めずに描いて `Game/resources/*.png` に保存 → `sprite.Initialize("resources/xxx.png", …)` で Release でも読める。保存時、読込済みのパスなら `TextureManager::Reload` で即差替（再起動不要）。操作一覧は docs/SpriteEditor.md。
- **Engine::SpriteCanvas**〔Release 除外〕— エディタの中身（RGBA8 配列と編集処理。ImGui/GPU 非依存）。自前ツールで画像処理だけ使える。ピクセルは `0xAABBGGRR`、アルファ 0 が未塗り — Engine/Tools/SpriteCanvas.h:22
- **SpriteCanvas::Create(w,h) / GetWidth / GetHeight / IsInside(x,y) / GetPixel(x,y) / GetCompositePixel(x,y) / GetComposite() / GetRevision()** — 作成・大きさ・1 px 取得・合成結果・更新カウンタ — Engine/Tools/SpriteCanvas.h:58-75
- **SpriteCanvas::BeginStroke / CanUndo / Undo / DrawLine(from,to,size,color) / EraseLine / FloodFill(at,color) / FloodErase(at)** — 履歴・ペン・消しゴム・塗りつぶし — Engine/Tools/SpriteCanvas.h:80-103
- **SpriteCanvas::HasSelection / IsSelecting / GetSelection / BeginSelection / UpdateSelection / EndSelection / MoveSelection(dx,dy) / CommitSelection** — 矩形選択と移動 — Engine/Tools/SpriteCanvas.h:108-130
- **SpriteCanvas::Resize(w,h) / Save(path, ImageFormat, error) / Load(path, error)** — 中心基準リサイズ、PNG/JPG 保存、読込（1024 まで） — Engine/Tools/SpriteCanvas.h:136-146
- 無いもの（エディタ）: グラデーション、レイヤー、アニメ（複数フレーム）、日本語表示。

---

## 4. 3D

### Object3D（Engine/Rendering/Object3D.h）= Model ＋ WorldTransform ＋ Material を 1 つにしたもの
- **Object3D::Initialize(Primitive, LightingMode=kHalfLambert)** — 組み込み形状（白テクスチャ）。`SetColor` で色付け。メッシュは形状ごとに共有 — Engine/Rendering/Object3D.h:40
- **Object3D::Initialize(const std::string& objFilePath, LightingMode=kHalfLambert)** — OBJ 読込（同じパスは 1 回だけ・共有）。mtl の map_Kd をサブメッシュごとに自動読込 — Engine/Rendering/Object3D.h:47
- **Object3D::Initialize(Mesh*, uint32_t textureHandle, LightingMode=kHalfLambert)** — 自作メッシュ（非所有。Object3D より長生きさせる） — Engine/Rendering/Object3D.h:53
- **Object3D::Update()** — ワールド行列 `S*R*T`・マテリアル・UV を CB へ書き込み、境界球を更新。Draw 前に毎フレーム必須 — Engine/Rendering/Object3D.h:59
- **Object3D::Draw() const** — 描画（視錐台の外なら何もしない） — Engine/Rendering/Object3D.h:64
- **Object3D::CalcWorldBoundingSphere()** — ワールド空間の境界球 `Sphere`（半径は scale の最大成分で拡大）。簡易当たり判定・ピッキングに — Engine/Rendering/Object3D.h:67
- **Object3D::GetTransform()** — `Transform3D&`（scale / rotate[rad, X→Y→Z] / translate） — Engine/Rendering/Object3D.h:69-70
- **Object3D::GetMaterialCount() / GetMaterial(index=0)** — マテリアル数（サブメッシュ数 or 1）/ 色・LightingMode の編集 — Engine/Rendering/Object3D.h:73-76
- **Object3D::SetColor(const Vector4&) / SetColor(uint32_t)** — 全マテリアルの色（テクスチャに乗算。a<1 で半透明＝不透明の後に奥から描く） — Engine/Rendering/Object3D.h:79-81
- **Object3D::GetUVTransform(index=0)** — UV スクロールなど — Engine/Rendering/Object3D.h:84
- **Object3D::SetTextureHandle(uint32_t) / GetTextureHandle()** — 全サブメッシュを一括上書き / 取得 — Engine/Rendering/Object3D.h:87-91
- **Object3D::SetTexture(const std::string&)** — パスから読み込んで上書き — Engine/Rendering/Object3D.h:94
- **Object3D::ClearTextureOverride() / IsTextureOverridden()** — mtl のテクスチャへ戻す / 上書き中か — Engine/Rendering/Object3D.h:97-98
- **Object3D::GetVisibility() / GetMesh()** — 直近 Draw のカリング結果 / 共有メッシュ — Engine/Rendering/Object3D.h:101-103
- **struct Transform3D { Vector3 scale, rotate, translate; }** — ファイル名が TransformData3D.h な点に注意 — Engine/Rendering/TransformData3D.h:6
- **enum class Primitive { kCube, kSphere, kPlane }** — 全て原点中心・1×1×1 に収まる（scale がそのまま大きさ。kPlane は上からしか見えない） — Engine/Rendering/Primitive.h:10
- **enum class LightingMode { kNone, kLambert, kHalfLambert }** / **struct Material { Vector4 color; LightingMode lightingMode; Matrix4x4 uvTransform; SetColor(Vector4|uint32_t) }** — Engine/Rendering/Material.h:10-25
- **親子付け: 無し**。Object3D は毎回 `transform_` から行列を作り（Object3D.cpp:79）、外からワールド行列を渡す口が無い。親子にするなら毎フレーム親の行列で子のローカル位置を `Transform()` して translate に書き戻す（回転の合成は自前）。
- 無いもの: ビルボード、インスタンス描画、パーティクル、アニメーション / スキニング、影、法線マップ、頂点カラー、オブジェクト単位の PSO 切替 API（自前描画なら `PipelineManager::SetPipeline` ＋ `RenderContext::GetCommandList()` で可能）。

### Mesh / MeshManager（自作形状・先読み用）
- **Mesh::Create(device, vertices, count)** / **Mesh::Create(device, vertices, count, indices, indexCount)** — 頂点（＋インデックス）から作成。時計回りが表 — Engine/Rendering/Mesh.h:36-42
- **Mesh::CreateFromObj(device, dir, filename) / CreateSphere(device, subdivision) / CreateCube(device)** — Engine/Rendering/Mesh.h:45-51
- **Mesh::Draw(cmd) / DrawSubMesh(cmd, index) / GetSubMeshCount() / GetSubMesh(index)** — 自前描画用 — Engine/Rendering/Mesh.h:54-63
- **Mesh::GetMappedVertices() / GetVertexCount() / GetLocalCenter() / GetLocalRadius() / RecomputeBoundingSphere()** — 頂点の直接編集（共有メッシュなので全オブジェクトに影響） — Engine/Rendering/Mesh.h:66-75
- **MeshManager::GetInstance() / Load(objFilePath) / GetPrimitive(Primitive)** — 先読み・キャッシュ（終了まで保持） — Engine/Rendering/MeshManager.h:22-31
- **struct VertexData { Vector4 position; Vector2 texcoord; Vector3 normal; }** — Engine/Rendering/VertexData.h:7
- **GenerateSphereVertices(subdivision) / GenerateCubeVertices() / GenerateCubeIndices() / GeneratePlaneVertices() / GeneratePlaneIndices()** — 組み込み形状の頂点列 — Engine/Geometry/GeometryGenerator.h:11-24

### Camera（Engine/Camera/Camera.h。シーンでは `camera_`）
- **Camera::GetTransform()** — translate=位置、rotate=向き（rad。x を正で下、y を正で右を向く）。scale は 1 のまま — Engine/Camera/Camera.h:21-22
- **Camera::SetFovY(float) / GetFovY()** — 縦画角 rad（既定 0.45 ≒ 26°） — Engine/Camera/Camera.h:24-25
- **Camera::SetClipPlanes(near, far) / GetNearClip() / GetFarClip()** — 既定 0.1〜100（広いステージは伸ばす） — Engine/Camera/Camera.h:27-32
- **Camera::GetViewMatrix()** — `Inverse(S*R*T)` — Engine/Camera/Camera.h:16
- **Camera::GetProjectionMatrix(aspectRatio)** — 透視投影のみ — Engine/Camera/Camera.h:19
- **正射影カメラ: 無し**（BaseScene::Update が `camera_.GetProjectionMatrix` を固定で使う。BaseScene.cpp:37。差し替えられるのはビュー行列だけ）。
- **注視点（LookAt）指定: 無し**。rotate を自分で求める（横視点なら rotate=0 で +Z を向かせ、translate を追従させれば足りる）。
- **スクリーン座標 ↔ ワールド座標の変換関数: 無し**。材料は揃っている: `Inverse(GetViewMatrix()*GetProjectionMatrix())` と `Transform()`、NDC は `GetRenderAreaX/Y/Width/Height` から計算（Engine/Camera/DebugCamera.cpp:66-77 に内部実装例: near/far の 2 点を逆変換してレイを作る）。ワールド→スクリーンは `view*proj*MakeViewportMatrix(...)` で `Transform`。
- **Engine::DebugCamera**〔Release 除外〕— マウスで見回す開発用カメラ。Enter で ON/OFF、左クリックで対象をピック — Engine/Camera/DebugCamera.h:18
- **DebugCamera::Update(targets, viewX, viewY, viewW, viewH, projection, blockMouse)** / **IsEnabled()** / **GetViewMatrix()** / **DrawImGui()**〔USE_IMGUI〕 — 組み込み方は EngineReference「DebugCamera」（`CalcViewMatrix` をオーバーライド） — Engine/Camera/DebugCamera.h:38-55
- **struct DebugCamera::PickTarget { Vector3 center; float radius; }** — Engine/Camera/DebugCamera.h:23
- 無いもの: カメラシェイク・追従・移動範囲制限のヘルパー、複数カメラの切替（`camera_` の値を書き換える）。

### ライト（シーンでは `directionalLight_` / `pointLights_`。値を変えれば Update で反映）
- **struct DirectionalLight { Vector4 color; Vector3 direction; float intensity; int32_t enabled; SetColor(Vector4|uint32_t) }** — 既定 白・(0,-1,0)・1・有効 — Engine/Light/DirectionalLight.h:12-22
- **struct PointLight { Vector4 color; Vector3 position; float intensity; float radius; float decay; int32_t enabled; SetColor }** — 既定 OFF — Engine/Light/PointLight.h:13-25
- **kMaxPointLightCount = 4 / struct PointLightGroup { PointLight lights[4]; }** — Engine/Light/PointLight.h:28-33
- 無いもの: スポットライト、影、環境光の調整（シェーダー固定。要確認）、フォグ。

### 線 / プリミティブのデバッグ描画（**有り**。Engine/Rendering/DebugDraw.h〔static〕。全構成でビルドされるので開発中限定にするなら `#ifndef NDEBUG`）
- **DebugDraw::DrawLine(start, end, color={1,1,1,1})** — 3D 線分 — Engine/Rendering/DebugDraw.h:26
- **DebugDraw::DrawSegment(Segment3D, color)** — Engine/Rendering/DebugDraw.h:29
- **DebugDraw::DrawSphere(Sphere, color)** — 緯度経度 12 分割の線 — Engine/Rendering/DebugDraw.h:32
- **DebugDraw::DrawAABB(AABB3D, color) / DrawOBB(OBB3D, color)** — 12 辺 — Engine/Rendering/DebugDraw.h:35-38
- **DebugDraw::DrawTriangle(Triangle3D, color) / DrawCapsule(Capsule3D, color) / DrawPlane(Plane3D, color, size=2)** — Engine/Rendering/DebugDraw.h:41-47
- **DebugDraw::DrawCurve(Curve, color, drawControlPoints=true)** — 32 分割の折れ線 — Engine/Rendering/DebugDraw.h:50
- **DebugDraw::DrawGrid(size=10, divisions=10, color)** — XZ グリッド — Engine/Rendering/DebugDraw.h:54
- 仕様: OnUpdate で毎フレーム積む（毎フレーム消える）。OnDraw の最後に深度テスト無しで描かれる。1 フレーム 65536 本まで（Engine/Rendering/DebugDraw.h:93）。色は Vector4（16 進は `ColorFromHex`）。
- 無いもの: **スクリーン座標（2D）の線・矩形・円**、塗りつぶし図形、矢印、XY 平面の円（DrawSphere か DrawLine の連打で代用）。

### 視錐台・描画コンテキスト（必要なら）
- **enum class FrustumVisibility { Outside, Intersect, Inside } / struct Frustum3D / struct Frustum2D** — Engine/Culling/FrustumCulling.h:30-46
- **MakeFrustumFromViewProjection(viewProjection) / MakeFrustumFromRect(min, max)** — Engine/Culling/FrustumCulling.h:52-55
- **ClassifyFrustum(Frustum3D, Sphere|AABB3D|OBB3D|Segment3D|Triangle3D)** / **ClassifyFrustum(Frustum2D, Circle|AABB2D|OBB2D|Segment2D|Triangle2D)** / **IsVisible(FrustumVisibility)** — 「画面外の敵は処理を省く」用途 — Engine/Culling/FrustumCulling.h:58-74
- **RenderContext::GetInstance() / GetCommandList() / GetFrustum() / GetScreenWidth() / GetScreenHeight()** — 自前描画の入口 — Engine/Rendering/RenderContext.h:17-38
- **PipelineManager::Pipeline { kStandard, kNoCull, kLine } / RootParameter / SetPipeline(cmd, pipeline) / GetRootSignature()** — PSO 切替（描いたら kStandard に戻す） — Engine/Graphics/PipelineManager.h:18-52
- **ConstantBuffer<T>::Create(device, slotCount) / Write(slot, data) / GetGPUAddress(slot) / Reset()** — 自前の定数バッファ — Engine/Rendering/ConstantBuffer.h:24-48
- **DirectXCore::GetInstance() / GetDevice() / GetFrameIndex() / kFramesInFlight** — 自作 Mesh / CB に必要 — Engine/Core/DirectXCore.h:19-41
- 立体視・視線追跡（StereoRenderer / StereoCamera / EyeTracker / FaceTracker / CameraCapture / GazePacket）は既定 OFF で、今回のゲームでは触らない（一覧は EngineReference.html「立体視・視線追跡」）。

---

## 5. アセット読込

### モデル
- 形式: **OBJ（＋ mtl の map_Kd のみ）**。面は三角形のみ（四角以上は assert。Blender は Triangulate ON）。頂点書式は 位置 / 位置+UV / 位置+法線 / 位置+UV+法線 に対応、無い UV は (0,0)、無い法線は (0,0,-1)。読込時に Z 反転・面の向き反転・V 反転（右手系→左手系） — Engine/Geometry/LoadObjFile.h:33-38
- **LoadObjFile(directoryPath, filename) → ModelData / LoadMaterialTemplateFile(dir, filename)** — 低レベル API（ふだんは Object3D::Initialize 経由） — Engine/Geometry/LoadObjFile.h:38-44
- **struct ModelData { std::vector<VertexData> vertices; std::vector<SubMeshData> subMeshes; std::unordered_map<std::string, MaterialData> materials; }** — Engine/Geometry/LoadObjFile.h:27
- mtl は OBJ と同じフォルダに置き、map_Kd のパスもそのフォルダからの相対。FBX / glTF / アニメ付きモデル: 無し。

### テクスチャ（Engine/Graphics/TextureManager.h〔シングルトン〕）
- **TextureManager::GetInstance()** — Engine/Graphics/TextureManager.h:24
- **TextureManager::Load(filepath) → uint32_t** — PNG / JPEG / BMP など WIC で読める形式（DDS 不可）。sRGB・ミップマップ自動。同一文字列パスはキャッシュ（`"resources/a.png"` と `"./resources/a.png"` は別扱い）。無いと assert — Engine/Graphics/TextureManager.h:36
- **TextureManager::Reload(filepath)** — 読み直して同じハンドルの中身を差替（GPU 完了待ちあり。毎フレーム不可） — Engine/Graphics/TextureManager.h:43
- **TextureManager::IsLoaded(filepath)** — Engine/Graphics/TextureManager.h:46
- **TextureManager::GetWhiteTexture()** — 1×1 白 — Engine/Graphics/TextureManager.h:52
- **TextureManager::CreateDynamic(key, w, h) / UpdateDynamic(handle, pixels) / ResizeDynamic(handle, w, h)** — CPU から書き換えるテクスチャ（RGBA8 sRGB、`0xAABBGGRR`）。OnUpdate / OnDrawImGui から呼ぶ（OnDraw 不可）。手描きエフェクトや動的 UI に使える — Engine/Graphics/TextureManager.h:61-74
- **TextureManager::GetSrvHandleGPU(handle)** — ImGui::Image や自前描画用 — Engine/Graphics/TextureManager.h:83
- **TextureManager::GetWidth(handle) / GetHeight(handle)** — 画像の実寸（スプライトを等倍で出すとき） — Engine/Graphics/TextureManager.h:86-87
- 制約: SRV ヒープ 128 枠（ImGui・立体視も使うので実質 120 枚弱）。個別解放は無し（終了まで保持）。アトラス・サンプラー変更（バイリニア WRAP 固定）・ゲーム内 Sprite のポイントサンプリング（ImGui 内だけ可）・DDS は無し。

### 音声ファイル
- **.wav（PCM、RIFF の fmt/data チャンク）のみ**。mp3 / ogg 不可。詳細は 6.

### パス規約
- 実行時カレント＝`Game/`。アセットは `Game/resources/` に置き `"resources/xxx.png"` と書く（docs/EngineGameWorkflow.md「アセットと実行時のカレントディレクトリ」）。現状 `Game/resources/` は `.gitkeep` のみ（モデル・画像はまだ無い）。
- シェーダーは `Shaders/` → `../Shaders/` の順に探索。配布時は exe・dxcompiler.dll・dxil.dll・Shaders/・resources/ を同じフォルダへ。
- ファイルが無いときは assert で止まる（Debug / Development）。Release は NDEBUG で assert が消えるため挙動不定（要確認）→ パスは開発構成で確認する。
- 読込はすべて同期（ロード画面・非同期読込の仕組みは無し）。

---

## 6. 音声（**有り**。Engine/Audio/Audio.h〔シングルトン〕、XAudio2）
- **Audio::GetInstance()** — Engine/Audio/Audio.h:21
- **Audio::LoadWave(filename) → size_t** — .wav 読込。**キャッシュ無し**（呼ぶたびに増える。シーンの OnInitialize で毎回読むと溜まる → アプリで 1 回だけ / static で保持） — Engine/Audio/Audio.h:34
- **Audio::Play(soundHandle)** — 頭から再生。同じハンドルが再生中なら止めて鳴らし直す（同じ音は重ならない。重ねたいなら同じファイルを別ハンドルで読む） — Engine/Audio/Audio.h:39
- **Audio::SetVolume(soundHandle, volume)** — 0=無音、1=原音、>1 増幅 — Engine/Audio/Audio.h:44
- **無いもの**: Stop / Pause / ループ再生（BGM）/ IsPlaying / フェード / ピッチ / パン / マスター音量 / 個別解放 / ストリーミング（Engine/Audio/Audio.cpp:179-196 の Stop・FlushSourceBuffers は Play 内部でしか使われない）。source voice は private なのでゲーム側からは追加できず、BGM ループが要るなら「エンジン側に機能を足す」案件（EngineReference の注記どおり）。

---

## 7. 数学ユーティリティ（すべて `namespace Engine` の struct / 自由関数）

### 型
- **struct Vector2 { float x, y; }** — 演算子: `a+b  a-b  -a  a*s  a/s  a+=b  a-=b  a*=s  s*a` — Engine/Math/Vector2.h:8-22
- **struct Vector3 { float x, y, z; }** — 同じ演算子 — Engine/Math/Vector3.h:8-22
- **struct Vector4 { float x, y, z, w; }** — 演算子無し（色・同次座標の受け渡し用） — Engine/Math/Vector4.h:8
- **struct Matrix4x4 { float m[4][4]; operator+ - * *= }** — 行優先・行ベクトル（`v*M`、`world=S*R*T`、`WVP=world*view*proj`、平行移動は m[3][0..2]） — Engine/Math/Matrix4x4.h:11-19
- **struct Transform3D** — 4. 参照 — Engine/Rendering/TransformData3D.h:6
- **ColorFromHex(uint32_t 0xRRGGBBAA) → Vector4**〔constexpr〕— 必ず 8 桁（6 桁だと色がずれる） — Engine/Math/Color.h:14
- 無いもの: `==` / `!=`、ベクトル同士の成分積、`/=`、Vector4 の演算子、**Vector2 用の数学関数（Dot / Length / Normalize / Lerp はすべて Vector3 専用）**。

### 行列（Engine/Math/Matrix4x4.h）
- **Add(m1, m2) / Subtract(m1, m2) / Multiply(m1, m2)** — 成分和・差 / 積 `m1*m2` — Engine/Math/Matrix4x4.h:29 / :37 / :45
- **Inverse(m)** — 逆行列（特異なら単位行列） — Engine/Math/Matrix4x4.h:52
- **Transpose(m)** — 転置 — Engine/Math/Matrix4x4.h:59
- **MakeIdentity4x4()** — Engine/Math/Matrix4x4.h:65
- **MakeTranslateMatrix(Vector3)** — Engine/Math/Matrix4x4.h:76
- **MakeScaleMatrix(Vector3)** — Engine/Math/Matrix4x4.h:83
- **MakeRotateXMatrix(rad) / MakeRotateYMatrix(rad) / MakeRotateZMatrix(rad)** — Engine/Math/Matrix4x4.h:98 / :105 / :112
- **MakeAffineMatrix(scale, rotate, translate)** — `S*Rx*Ry*Rz*T`（Object3D と同じ） — Engine/Math/Matrix4x4.h:121
- **Transform(Vector3, Matrix4x4)** — 点の変換（w=1、透視除算あり。w=0 で assert）。方向ベクトルは回転行列だけを渡す — Engine/Math/Matrix4x4.h:91
- **MakePerspectiveFovMatrix(fovY, aspect, near, far)** — 左手系・深度 0..1 — Engine/Math/Matrix4x4.h:135
- **MakeOrthographicMatrix(left, top, right, bottom, near, far)** — Engine/Math/Matrix4x4.h:147
- **MakeViewportMatrix(left, top, width, height, minDepth, maxDepth)** — NDC→スクリーン（Y 下向き）。ワールド→スクリーン変換の部品 — Engine/Math/Matrix4x4.h:159
- 無いもの: 任意軸回転、LookAt / ビュー行列生成、クォータニオン、行列から位置・回転の取り出し、TransformNormal。

### ベクトル（Vector3 専用。Engine/Math/Matrix4x4.h に宣言）
- **Cross(v1, v2)** — 外積 — Engine/Math/Matrix4x4.h:167
- **Dot(v1, v2)** — 内積 — Engine/Math/Matrix4x4.h:179
- **Add(v1, v2) / Subtract(v1, v2) / Multiply(v, s) / Multiply(s, v)** — 演算子と同じ — Engine/Math/Matrix4x4.h:187 / :195 / :203 / :223
- **Project(v1, v2)** — v2 方向への射影 — Engine/Math/Matrix4x4.h:211
- **Length(v)** — Engine/Math/Matrix4x4.h:230
- **Normalize(v)** — ゼロベクトルならゼロ — Engine/Math/Matrix4x4.h:237
- **Reflect(input, normal)** — 反射（normal は単位ベクトル）。跳ね返りに — Engine/Math/Matrix4x4.h:245
- **Lerp(float start, float end, float t) / Lerp(Vector3, Vector3, t)** — 線形補間（0〜1 の外は外挿）。カメラ追従 `pos = Lerp(pos, target, 0.1f)` — Engine/Math/Matrix4x4.h:258 / :267
- 無いもの: Distance、Slerp、Clamp / Saturate、角度⇔ベクトル（atan2 のラッパ）、**Vector2 版すべて**（マウス狙いの 2D 計算は自前か z=0 の Vector3 で代用）。

### 形状（Engine/Math/Shapes.h）
- **Sphere { center, radius }** :20 / **AABB3D { min, max }** :26 / **OBB3D { center, orientations[3], size(半幅) }** :33 / **Segment3D { start, end }** :40 / **Triangle3D { v0, v1, v2 }** :46 / **Capsule3D { segment, radius }** :53
- **Circle { center, radius }** :61 / **AABB2D { min, max }** :67 / **OBB2D { center, orientations[2], size }** :73 / **Segment2D { start, end }** :80 / **Triangle2D { v0, v1, v2 }** :86
- **Plane3D { normal, distance }**（`dot(n,p)+d=0`）:95 / **Plane2D { normal, distance }** :101

### 当たり判定（Engine/Math/Collision.h。**3D のみ、戻り値は bool のみ**。接していても true）
- **MakeAABB(center, halfSize)** — kCube を scale s で置いたなら `MakeAABB(translate, s*0.5f)` — Engine/Math/Collision.h:21
- **MakeOBB(center, rotate, halfSize)** — 回転は rad・X→Y→Z — Engine/Math/Collision.h:24
- **MakePlane(normal, point)** — 法線は正規化される — Engine/Math/Collision.h:27
- **SignedDistance(point, Plane3D)** — めり込み量に — Engine/Math/Collision.h:30
- **ClosestPoint(point, Segment3D)** — 線分上の最近点 — Engine/Math/Collision.h:33
- **IsCollision** の組み合わせ: Sphere–Sphere :38 / Sphere–Plane3D :41 / Capsule3D–Plane3D :44 / Segment3D–Plane3D :47 / Triangle3D–Segment3D :50 / AABB3D–AABB3D :53 / AABB3D–Sphere :56 / AABB3D–Segment3D :59 / OBB3D–Sphere :62 / OBB3D–Segment3D :65 / OBB3D–OBB3D（分離軸 15 本）:68 / OBB3D–AABB3D :71、引数を逆にした inline 版 :74-82 — Engine/Math/Collision.h
- 無いもの: **2D 形状同士の判定（Circle / AABB2D などは型だけ）**、Sphere–Capsule、Capsule–Capsule、Capsule–AABB/OBB、Sphere–Triangle、Ray 型・レイキャスト（線分で代用）、**めり込み量・接触法線・押し出しベクトル**、連続衝突（Capsule–Plane 以外）、コライダー / マネージャー / コールバック / レイヤーマスク。

### 曲線（Engine/Math/Curve.h）
- **Bezier(p0, p1, p2, t)** — 2 次ベジェ上の点 — Engine/Math/Curve.h:16
- **Curve(p0, p1, p2) / Curve()** — 制御点 3 つ（引数なしは動作確認用の固定 3 点） — Engine/Math/Curve.h:26-29
- **Curve::GetPoint(t)** — 0〜1 — Engine/Math/Curve.h:32
- **Curve::GetControlPoints() / SetControlPoint(index, point)** — Engine/Math/Curve.h:34-37
- **Curve::DrawImGui(label)**〔USE_IMGUI〕— 制御点編集 UI を呼び出し側のウィンドウに差し込む — Engine/Math/Curve.h:41
- 無いもの: 3 次ベジェ、Catmull-Rom、N 点スプライン、弧長パラメータ化（等速移動）。

### 無いもの（数学全般）
- イージング関数（EaseIn/Out 等）、乱数（`<random>` を直接使う）、Clamp / Min / Max / Saturate / Sign / Wrap、円周率定数（`kPi` は Engine/Rendering/DebugDraw.cpp の内部定数のみ。`std::numbers::pi` を使う）、度⇔ラジアン変換、角度の正規化、2D 回転、Vector2 の全数学関数、スクリーン⇔ワールド変換。

---

## 8. ユーティリティ

### ImGui
- 有効条件: **Debug・Development のみ**（`USE_IMGUI`。Engine/Engine.props:39, :53）。Release ではヘッダごと存在しないので、`ImGui::` を使う行・`#include "externals/imgui/imgui.h"`・SpriteEditor / ImGuiManager / DebugCamera::DrawImGui の利用はすべて `#ifdef USE_IMGUI` で囲む。
- 書く場所: シーンの `OnDrawImGui()`（OnUpdate より先に呼ばれる）。ImGui のフォントは ASCII のみ。
- **ImGuiManager::GetInstance()**〔シングルトン, USE_IMGUI〕 — Engine/Core/ImGuiManager.h:34
- **ImGuiManager::SetDefaultDockLayout(topRight, bottomRight)** — 初回起動時のドッキング配置（ウィンドウ名で指定。Framework::Initialize 後・最初のフレーム前） — Engine/Core/ImGuiManager.h:42
- **ImGuiManager::GetGameArea()** — ゲーム描画先（BaseScene::GetRenderArea* と同じ値） — Engine/Core/ImGuiManager.h:58
- **ImGuiManager::PushPointSampling(ImDrawList*) / PopPointSampling(ImDrawList*)** — ImGui 内の画像を点サンプリングで拡大 — Engine/Core/ImGuiManager.h:64-65
- 画像表示: `ImGui::Image((ImTextureID)TextureManager::GetInstance()->GetSrvHandleGPU(h).ptr, ImVec2(...))`。
- ImGui 操作中は Input のキー / ボタン / ホイールが 0 になる（2. 参照）。

### ログ・アサート・クラッシュ
- **Log(const std::string& message)** — VS 出力ウィンドウ＋ `Game/logs/YYYYMMDD_HHMMSS.log`（起動ごとに新規）。数値は `std::format` で — Engine/Diagnostics/Log.h:15（実装 Engine/Diagnostics/Log.cpp:21-34）
- **InitializeLogFile()**〔エンジンが呼ぶ〕 — Engine/Diagnostics/Log.h:12
- アサート: エンジン独自マクロは無し。標準 `assert`（Debug・Development で有効、Release は NDEBUG で無効）。
- **ExportDump(EXCEPTION_POINTERS*)**〔エンジンが登録〕— クラッシュ時に `Game/Dumps/*.dmp` を出力（VS で開くとコールスタックが見える） — Engine/Diagnostics/CrashHandler.h:6
- 無いもの: ログレベル、画面内ログ表示、`#ifdef` 無しで消えるデバッグログマクロ。

### 時間（Engine/Core/Time.h〔static〕）
- **Time::kTargetFrameRate = 60.0f** — 更新は毎秒 60 回固定（Present 後に 1/60 秒まで待つ。Engine/Core/Time.cpp:41-74） — Engine/Core/Time.h:14
- **Time::GetDeltaTime()** — 前フレームからの実測秒（約 1/60、上限 `kMaxDeltaTime = 0.1`） — Engine/Core/Time.h:17（上限 :34）
- **Time::GetTotalTime()** — 起動からの秒 — Engine/Core/Time.h:20
- 「1 フレームあたりの量」で書いてよい（Game/ の雛形はその流儀）。秒単位なら `GetDeltaTime()` を掛ける。
- 無いもの: フレームカウンタ、FPS 取得・表示、タイムスケール（スロー / ヒットストップ / ポーズ）、ストップウォッチ / タイマークラス、可変フレームレート設定。

### ウィンドウ（Engine/Core/WinApp.h〔シングルトン〕）
- **WinApp::kClientWidth = 1280 / kClientHeight = 720** — 起動時サイズ＝スプライトの基準解像度 — Engine/Core/WinApp.h:11-12
- **WinApp::GetInstance() / GetClientWidth() / GetClientHeight()** — 今のクライアント領域（ImGui 込み） — Engine/Core/WinApp.h:15-22
- **WinApp::SetFullscreen(bool) / ToggleFullscreen() / IsFullscreen()** — ボーダレス全画面（F11 でも切り替わる） — Engine/Core/WinApp.h:32-34
- **WinApp::SetTitle(const std::wstring&) / GetTitle()** — スコア表示などに — Engine/Core/WinApp.h:38-39
- **WinApp::GetHwnd()** — Win32 API を直接使うとき（カーソル制御など） — Engine/Core/WinApp.h:41
- 背景クリア色は固定（変更 API 無し。DirectXCore.cpp）。ウィンドウアイコン・カーソル変更の API 無し。

### ファイル読込・文字列
- CSV / JSON / INI / 独自レベルデータの読込: **無し**（エンジン内の `std::ifstream` は Audio / OBJ 専用）。ゲーム側で `<fstream>` / `<sstream>` / `std::filesystem` を直接使う（C++20）。
- **ConvertString(const std::string&) → std::wstring / ConvertString(const std::wstring&) → std::string** — UTF-8⇔UTF-16（このヘッダは `<string>` を含まないので先に include） — Engine/String/ConvertString.h:5-6
- `std::format` は使用可（Log.cpp で使用）。
- セーブデータ、設定ファイル、スクリーンショット: 無し。

---

## 9. エンジンに無いもの（ゲーム側で実装が必要）—「ありそうで無い」を含む
- **当たり判定**: 2D 形状同士（Circle–Circle、AABB2D–AABB2D 等。型 Shapes.h は有るが IsCollision が無い）。3D でも Sphere–Capsule / Capsule–Capsule / Capsule–箱 / Sphere–Triangle。Ray 型・レイキャスト。めり込み量・接触法線・押し出し。コライダーの登録・総当たり・コールバック・レイヤー。→ 横視点ならワールド XY で Sphere / AABB3D 判定（z を揃える）が最短。
- **スクリーン座標 ↔ ワールド座標**: 関数無し（マウスで狙うブーメランに必須）。`Inverse(GetViewMatrix()*GetProjectionMatrix())` ＋ `Transform` ＋ `GetRenderArea*` で自前実装（Engine/Camera/DebugCamera.cpp:66-77 が参考。ゲームの Z=0 平面との交点を取る）。ワールド→スクリーンも無し（`MakeViewportMatrix` で自前）。
- **線描画**: 3D の DebugDraw は有り。**2D（スクリーン座標）の線・矩形・円は無し**。狙い線は 3D 線で代用可（深度無視・全構成でビルドされるが、見た目は 1px の線のみ。製品用の太い線は自前 PSO が必要）。
- **パーティクル / インスタンス描画 / ビルボード / 残像 / トレイル**: 無し。少数なら Object3D や Sprite を個別に持つ。
- **シーン管理**: 有り（SceneManager / BaseScene）。ただしフェード等の遷移、シーンスタック、ポーズ、シーン間のデータ受け渡し（コンストラクタ引数以外）は無し。
- **音声**: 読込・再生・音量のみ。Stop / ループ BGM / IsPlaying / フェード / キャッシュは無し（ゲーム側からは追加不能）。
- **イージング / 乱数 / Clamp / PI / 度ラジアン / Vector2 の Dot・Length・Normalize・Lerp / Distance / LookAt / クォータニオン / 3 次ベジェ・スプライン**: 無し。
- **JSON / CSV / INI 読込、レベルデータ、セーブ / 設定ファイル**: 無し。
- **テキスト・数字描画 / フォント**: 無し（数字スプライトを UV で切り出す等で代用）。
- **Sprite**: アンカー、反転フラグ、レイヤー順、テクスチャハンドル取得、スプライトシート再生、9 スライス: 無し。
- **Object3D**: 親子付け / 外部ワールド行列 / 正射影カメラ / 注視点 / カメラシェイク等のヘルパー: 無し。
- **ライト**: スポット・影・フォグ・環境光調整: 無し。
- **ポストエフェクト / ゲーム用レンダーターゲット / 画面フェード / スクリーンショット**: 無し。
- **タイマー・時間制御**: フレームカウンタ・FPS 表示・タイムスケール・ヒットストップ・ストップウォッチ: 無し。
- **入力**: カーソル表示 / 非表示・固定、文字入力、キーコンフィグ: 無し。
- **ゲームオブジェクト基盤**: GameObject 基底 / コンポーネント / オブジェクト管理（生成・破棄・一括 Update / Draw）/ 状態機械（State パターン）: 無し。
- **テクスチャ**: アトラス、個別解放、サンプラー切替（ポイントサンプリング）、DDS: 無し。
- **その他**: 背景色変更、非同期ロード、ウィンドウアイコン、パッドのキー割当補助、2D 専用カメラ。

---

## 10. Game/ の現在の雛形
- **Game/main.cpp** — `WinMain` で `MyGame game; game.Run();` のみ — Game/main.cpp:6-12
- **Game/MyGame.h / .cpp** — `Engine::Framework` を継承。`Initialize()` だけオーバーライドし、`SetWindowTitle(L"My Game")` → `Framework::Initialize()` → `ChangeScene<TitleScene>()` — Game/MyGame.cpp:5-14
- **Game/Scene/TitleScene** — `Engine::BaseScene` 継承。OnInitialize: カメラを z=-6 へ、平行光源の向きを設定、仮ロゴ（kCube・黄色）。OnUpdate: Enter / Space / パッド A の Trigger で `ChangeScene<GameScene>()`、立方体を回して `Update()`。OnDraw: `logo_.Draw()`。OnDrawImGui: 案内テキスト — Game/Scene/TitleScene.cpp:12-45
- **Game/Scene/GameScene** — OnInitialize: カメラ (0,3,-12)・下向き 0.15 rad、光源、地面（kPlane を scale 20×1×20・緑）、`player_.Initialize()`・`enemy_.Initialize()`。OnUpdate: Esc で `ChangeScene<TitleScene>()`、player / enemy / ground の Update。OnDraw: ground → enemy → player。OnDrawImGui: カメラの DragFloat3 と各オブジェクトの DrawImGui — Game/Scene/GameScene.cpp:12-60
- **Game/Object/Player** — Object3D（kCube・赤）1 個。A/D で左右移動（kSpeed=0.05/フレーム）、W の Trigger でジャンプ（初速 0.18、重力 0.01/フレーム、kGroundY=0.5 で着地）。`Initialize / Update / Draw / DrawImGui / GetPosition` — Game/Object/Player.h:10-41, Game/Object/Player.cpp:11-66
- **Game/Object/Enemy** — Player と同じ構造・同じ定数。接地したら即ジャンプを繰り返すだけ（左右移動・プレイヤー追尾はヘッダのコメントにあるが未実装） — Game/Object/Enemy.cpp:9-43
- シーン切替方式: ゲーム側独自の仕組みは無く、エンジンの `ChangeScene<T>()`（BaseScene / Framework 経由の SceneManager 予約制）をそのまま使用。シーンは毎回 new される。
- 全オブジェクトが「1 フレームあたりの量」で動く流儀（60 回/秒固定前提）。Game 側コードは `namespace Engine` に入れず、.cpp で `using namespace Engine;`。ImGui 部分は `#ifdef USE_IMGUI` で囲んである。
- `Game/resources` は空（.gitkeep のみ）。

---

## 11. ビルド / 構成メモ
- ソリューション: `CG3.slnx`（Engine.vcxproj、externals/DirectXTex、Game.vcxproj）。構成は **Debug / Development / Release**（x64）。出力は `x64\<構成>\`。
- 共通コンパイル設定（Engine/Engine.props を Game.vcxproj も import。Game/Game.vcxproj:57）: **警告レベル 4（/W4）・警告はエラー（/WX）・SDLCheck・ConformanceMode（/permissive-）・C++20・/utf-8・マルチプロセッサビルド・静的デバッグ CRT（MultiThreadedDebug）を全構成で** — Engine/Engine.props:23-32。未使用変数 / 引数で即ビルド失敗（引数名を消す or `[[maybe_unused]]`）。
- 構成の差（Engine/Engine.props:39-67、docs/EngineGameWorkflow.md 補足）:
  - **Debug**: `_DEBUG;_WINDOWS;USE_IMGUI`。ImGui・DebugCamera あり、assert 有効、エンジンも含め最適化なし。エンジン内部を追うとき。
  - **Development**: `_WINDOWS;USE_IMGUI`（NDEBUG 無し → assert 有効）。ImGui・DebugCamera あり。Game/ は最適化なし（/Od、Engine.props:60）、Engine.lib は最適化あり。ふだんの開発用。
  - **Release**: `NDEBUG;_WINDOWS`。ImGui・DebugCamera・SpriteEditor / SpriteCanvas・imgui の .cpp がビルドから除外（Engine/Engine.vcxproj:62-125）。assert 無効、プログラム全体の最適化（Game/Game.vcxproj:47）。ImGui 無しの見た目はこれで確認。
  - **`_DEBUG` は CRT の都合で Release 以外にも定義されるので分岐に使わない**。ImGui は `#ifdef USE_IMGUI`、開発中だけのコードは `#ifndef NDEBUG`。
- Windows.h の min/max マクロが有効のまま（Input.h / WinApp.h / Audio.h 経由）。`std::min/std::max` は比較式か `(std::min)(a, b)` で書く。
- ポストビルドで dxcompiler.dll / dxil.dll を exe の隣にコピー、DPI は PerMonitorHighDPIAware（Engine/Engine.props:80-86）。
- `.editorconfig`: UTF-8、IntelliSense のスペルチェック（識別子・コメント、en-us）をエラー扱い（除外辞書 `exclusion.dic`）、K&R の波かっこ。
- pre-commit フック（`.githooks/pre-commit`。`core.hooksPath=.githooks` 設定済み）: ゲーム制作ブランチ（Game/Game.vcxproj があるブランチ）では **Game/ 以外のファイルをコミットできない**（マージコミット中は除く。`--no-verify` で回避可）。
- ブランチ運用（docs/EngineGameWorkflow.md）: `master`=エンジンの主軸、`feature/〇〇`=エンジン機能、`AL4_develop`=ゲーム（Game/ だけ編集）。エンジン→ゲームの取り込みは `powershell -ExecutionPolicy Bypass -File tools/MergeEngine.ps1` のみ（`-EngineBranch feature/〇〇` で未マージ機能も可。VS のマージや素の `git merge` は使わない）。ゲーム→エンジンのマージ禁止。エンジン修正が要るなら master から feature/ を切って Sandbox で確認 → master → MergeEngine。
- ゲーム側の書き方の約束（docs/EngineGameWorkflow.md「ゲーム側の書き方」＋ EngineReference「決まりごと」「つまずきやすい点」）: 左手系（+X 右・+Y 上・+Z 奥）、角度はラジアン、行列は行ベクトル、2D は左上原点・1280×720 基準、色は 0〜1 の Vector4 か 0xRRGGBBAA（8 桁）、更新は 60 回/秒固定、Object3D / Sprite は毎フレーム Update → Draw、OnDraw でゲームの状態を変えない、半透明は不透明の後に奥から、スプライトは OnDraw の最後、Initialize 済みオブジェクトをコピーしない、テクスチャは 120 枚弱まで、音はアプリで 1 回だけ読む。
- CLAUDE.md: リポジトリ内に無し（ルート・`.claude/` とも）。従うべき方針はユーザーのメモリ側（動的管理は `std::unique_ptr` 優先、Windows.h の min/max 回避、制作中は Engine/ を変更しない、ユーザー向け文は日本語、Co-Authored-By を付けない）。
