# エンジン制作とゲーム制作の進め方

同じリポジトリの中で、エンジンとゲームをブランチで分けて作るための手順です。

## フォルダ構成

ソリューションとソース一式は `project/` に、ビルドで生成されるファイルは `generated/` に分けています。

| フォルダ | 中身 | 編集するブランチ |
|---|---|---|
| `project/CG3.slnx` | ソリューション | エンジン |
| `project/Engine/` | エンジン本体（静的ライブラリ `Engine.vcxproj`、共通ビルド設定 `Engine.props`） | エンジン |
| `project/Engine/Shaders/` | エンジンのシェーダー（HLSL。実行時にコンパイルする） | エンジン |
| `project/externals/` | 外部ライブラリ（DirectXTex・imgui）。他の人が書いたもの | エンジン |
| `project/Sandbox/` | エンジンの動作確認用アプリ。**エンジン制作ブランチにだけある** | エンジン |
| `project/Sandbox/resources/` | Sandbox が実行時に読み込むファイル（モデル・画像など） | エンジン |
| `project/Game/` | ゲーム本体。**ゲーム制作ブランチにだけある** | ゲーム |
| `project/Game/resources/` | ゲームが実行時に読み込むファイル | ゲーム |
| `generated/outputs/<構成>/` | ビルドの出力（exe・lib・dll）。**git 管理外** | － |
| `generated/obj/<プロジェクト>/<構成>/` | ビルドの中間ファイル。**git 管理外** | － |
| `tools/` | 補助ツール（`MergeEngine.ps1`、視線送信ツール） | エンジン |
| `docs/` | 手順書 | エンジン |

どちらのブランチでも `project/CG3.slnx` を開いて F5 で実行します。
エンジン制作ブランチでは `Sandbox`、ゲーム制作ブランチでは `Game` が起動します。

ソリューションエクスプローラーでは、`Engine` フォルダにエンジン、`externals` フォルダに外部ライブラリ（DirectXTex・imgui）、
その下にアプリ（`Sandbox` か `Game`）が並びます。imgui は Debug・Development だけでビルド・リンクされ、Release では使いません。

### #include の書き方

| 書く場所 | 書き方 | 例 |
|---|---|---|
| エンジン（`project/Engine/`） | そのファイルからの相対パス | `#include "../Core/WinApp.h"`、`#include "../../externals/imgui/imgui.h"` |
| アプリ（`project/Sandbox/`・`project/Game/`） | 追加のインクルードディレクトリ（`$(SolutionDir)`・`$(ProjectDir)`）からのパス | `#include "Engine/Core/WinApp.h"`、`#include "externals/imgui/imgui.h"`、`#include "Scene/GameScene.h"` |

エンジンはアプリの設定に頼らずにビルドできるよう相対パスで書きます。
アプリ側の追加のインクルードディレクトリは `project/Engine/Engine.props` でまとめて設定しています。

## ブランチ

`main` を主軸にして進めます。

| ブランチ | 役割 |
|---|---|
| `main` | 主軸。動作確認の済んだエンジン（Engine / Shaders / Sandbox など）。ここでは直接作業せず、`feature/〇〇` をマージして更新する |
| `feature/〇〇` | エンジンの機能を作るブランチ（例: `feature/Input`）。`main` から分岐し、Sandbox で動作確認できたら `main` へマージする |
| `AL4_develop` | ゲーム制作（`project/Game/` だけを編集する）。エンジンは `main` から取り込む |

`main` と `feature/〇〇` がエンジン制作ブランチ（`project/Sandbox/` がある）、`AL4_develop` がゲーム制作ブランチ（`project/Game/` がある）です。
変更の取り込みは **エンジン（`main`）→ ゲーム の一方通行** です。ゲームのブランチを `main` や `feature/〇〇` へマージしないでください。

## 最初に1回だけ

編集範囲のチェック（pre-commit フック）を有効にします。

```bash
git config core.hooksPath .githooks
```

- ゲーム制作ブランチで `project/Game/` 以外のファイルをコミットしようとすると止まります
- エンジン制作ブランチで `project/Game/` のファイルをコミットしようとすると止まります
- どうしても必要なときは `git commit --no-verify` でチェックを飛ばせます

## エンジンの機能を作る

1. `main` から `feature/〇〇` を作る（`git switch -c feature/〇〇 main`）
2. Engine / Sandbox を編集し、Sandbox で動作確認しながらコミットする
3. できあがったら `main` へマージする（`git switch main` → `git merge --no-ff feature/〇〇`）
4. ゲームで使うときは、ゲーム制作ブランチで `tools/MergeEngine.ps1` を実行して取り込む（次の節）

## エンジンの変更をゲームに取り込む

ゲーム制作ブランチ（`AL4_develop`）で次を実行します。

```bash
powershell -ExecutionPolicy Bypass -File tools/MergeEngine.ps1
```

`main` をマージし、ゲームに不要な `project/Sandbox/` は自動で取り除きます。
コンフリクトが残ったときは表示されたファイルを直して `git add` → `git commit` してください。
`main` にまだマージしていない機能を先に使いたいときは、末尾に `-EngineBranch feature/〇〇` を付けるとそのブランチから取り込めます。

> Visual Studio のマージ機能や `git merge` を直接使わず、このスクリプトを使ってください。
> git は `project/Game/Game.vcxproj` や `project/Game/main.cpp` を「Sandbox のファイルを名前変更したもの」とみなすことがあり、
> 普通にマージするとエンジン側での Sandbox の変更がゲームのプロジェクトに混ざってしまいます
> （スクリプトは名前変更の検出を切ってマージします）。

## ゲーム制作中にエンジンの修正が必要になったら

1. ゲーム側の作業をコミット（または `git stash`）する
2. `main` から `feature/〇〇` を作ってエンジンを修正し、Sandbox で動作確認してコミットする
3. `feature/〇〇` を `main` へマージする
4. `AL4_develop` に戻り、`tools/MergeEngine.ps1` で取り込む

2つのブランチを別々のフォルダに同時に出しておくと、切り替えのたびの再ビルドが要らなくなります。

```bash
git worktree add ../DirectXGame-game AL4_develop
```

## ゲーム側の書き方（よく使うもの）

| やりたいこと | 書き方 |
|---|---|
| ウィンドウのタイトル | `MyGame::Initialize` の中で `SetWindowTitle(L"タイトル");`（`Framework::Initialize()` より前） |
| 最初のシーン | `MyGame::Initialize` の中で `ChangeScene<TitleScene>();` |
| シーンを作る | `Engine::BaseScene` を継承し、`OnInitialize` / `OnUpdate` / `OnDraw`（/ `OnDrawImGui`）を書く |
| シーンを切り替える | シーンの中で `ChangeScene<GameScene>();`（次のフレームの頭で切り替わる） |
| カメラ・ライト | シーンの `camera_` / `directionalLight_` / `pointLights_` を変更する |
| 仮モデル | `object.Initialize(Engine::Primitive::kCube);` `object.SetColor({ 1, 0, 0, 1 });`（kCube / kSphere / kPlane） |
| OBJモデル | `object.Initialize("resources/player.obj");`（同じファイルは1回だけ読み込まれる） |
| 毎フレーム | `OnUpdate` で `object.Update();`、`OnDraw` で `object.Draw();` |
| 2D画像 | `sprite.Initialize("resources/title.png", { 幅, 高さ });` → `Update()` / `Draw()` |
| 2D画像をその場で描く | `Engine::SpriteEditor::GetInstance()->SetOpen(true);` で「Sprite Editor」ウィンドウを開いて描き、`sprite.SetTextureHandle(Engine::SpriteEditor::GetInstance()->GetTextureHandle());` で表示する（どちらも `#ifdef USE_IMGUI` の中）。気に入ったら PNG/JPG に保存（Debug / Development のみ。詳しくは `docs/SpriteEditor.md`） |
| 当たり判定 | `Engine::IsCollision(a, b)`（`#include "Engine/Math/Collision.h"`。球・AABB・OBB・線分など） |
| 曲線 | `Engine::Curve` の `GetPoint(t)`（`#include "Engine/Math/Curve.h"`） |
| デバッグ線 | `Engine::DebugDraw::DrawSphere(sphere, color);` など（`OnUpdate` で毎フレーム呼ぶ） |
| 時間 | 更新は毎秒60回に固定。秒単位の処理は `Engine::Time::GetDeltaTime()` |

## アセットと実行時のカレントディレクトリ

- ゲームのアセットは `project/Game/resources/` に置き、`"resources/xxx.png"` のように読み込みます
- VS から実行すると、カレントディレクトリはアプリのフォルダ（`project/Game/` や `project/Sandbox/`）になります
  （exe 自体は `generated/outputs/<構成>/` にあります）
- エンジンのシェーダーは `Shaders/` → `../Engine/Shaders/` の順に探します
- 配布するときは exe と dll（`dxcompiler.dll` / `dxil.dll`）、`Shaders/`（`project/Engine/Shaders/` の中身）、`resources/` を同じフォルダに置きます
- 実行時に作られるファイル（`imgui.ini`・`logs/`・`Dumps/`）はアプリのフォルダにでき、git 管理外です

## 補足

- ブランチを切り替えても、git 管理外のファイル（`imgui.ini`・`logs/`・`generated/` など）はそのまま残ります。
  ゲーム制作ブランチに `project/Sandbox/` フォルダが残って見えても、中身が管理外のファイルだけなら問題ありません
- ビルドは `project/CG3.slnx` から行います（出力先などの設定で `$(SolutionDir)` を使っているため、プロジェクト単体ではビルドしません）
- ビルド構成は3つで、ソリューションの構成はそれぞれ同じ名前のプロジェクト構成でビルドされます
  - `Debug` … ImGui・デバッグカメラあり、エンジンも含めて最適化なし。エンジンの中をデバッグするときに使う
  - `Development` … ふだんのゲーム開発用。ImGui・デバッグカメラあり、assert 有効。ゲーム（`project/Game/`・`project/Sandbox/`）のコードは最適化なしなので、ブレークポイントで止めて変数を見たりステップ実行したりできる。エンジン（`Engine.lib`）は最適化あり
  - `Release` … 提出・配布用。ImGui・デバッグカメラなし、すべて最適化あり（プログラム全体の最適化も）、assert なし（`NDEBUG`）、D3D12 デバッグレイヤーなし、CRT は通常の静的リンク（`/MT`。Debug・Development は `/MTd`）。ImGui のない見た目もこれで確かめる
