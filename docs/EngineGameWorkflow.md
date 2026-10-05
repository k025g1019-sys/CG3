# エンジン制作とゲーム制作の進め方

同じリポジトリの中で、エンジンとゲームをブランチで分けて作るための手順です。

## フォルダ構成

| フォルダ | 中身 | 編集するブランチ |
|---|---|---|
| `Engine/` | エンジン本体（静的ライブラリ `Engine.vcxproj`、共通ビルド設定 `Engine.props`） | エンジン |
| `Shaders/` | エンジンのシェーダー | エンジン |
| `externals/` | 外部ライブラリ（DirectXTex・ImGui） | エンジン |
| `tools/` | 補助ツール（`MergeEngine.ps1`、視線送信ツール） | エンジン |
| `Sandbox/` | エンジンの動作確認用アプリ。**エンジン制作ブランチにだけある** | エンジン |
| `Game/` | ゲーム本体。**ゲーム制作ブランチにだけある** | ゲーム |

どちらのブランチでも `CG3.slnx` を開いて F5 で実行します。
エンジン制作ブランチでは `Sandbox`、ゲーム制作ブランチでは `Game` が起動します。

## ブランチ

`master` を主軸にして進めます。

| ブランチ | 役割 |
|---|---|
| `master` | 主軸。動作確認の済んだエンジン（Engine / Shaders / Sandbox など）。ここでは直接作業せず、`feature/〇〇` をマージして更新する |
| `feature/〇〇` | エンジンの機能を作るブランチ（例: `feature/Input`）。`master` から分岐し、Sandbox で動作確認できたら `master` へマージする |
| `AL4_develop` | ゲーム制作（`Game/` だけを編集する）。エンジンは `master` から取り込む |

`master` と `feature/〇〇` がエンジン制作ブランチ（`Sandbox/` がある）、`AL4_develop` がゲーム制作ブランチ（`Game/` がある）です。
変更の取り込みは **エンジン（`master`）→ ゲーム の一方通行** です。ゲームのブランチを `master` や `feature/〇〇` へマージしないでください。

## 最初に1回だけ

編集範囲のチェック（pre-commit フック）を有効にします。

```bash
git config core.hooksPath .githooks
```

- ゲーム制作ブランチで `Game/` 以外のファイルをコミットしようとすると止まります
- エンジン制作ブランチで `Game/` のファイルをコミットしようとすると止まります
- どうしても必要なときは `git commit --no-verify` でチェックを飛ばせます

## エンジンの機能を作る

1. `master` から `feature/〇〇` を作る（`git switch -c feature/〇〇 master`）
2. Engine / Sandbox を編集し、Sandbox で動作確認しながらコミットする
3. できあがったら `master` へマージする（`git switch master` → `git merge --no-ff feature/〇〇`）
4. ゲームで使うときは、ゲーム制作ブランチで `tools/MergeEngine.ps1` を実行して取り込む（次の節）

## エンジンの変更をゲームに取り込む

ゲーム制作ブランチ（`AL4_develop`）で次を実行します。

```bash
powershell -ExecutionPolicy Bypass -File tools/MergeEngine.ps1
```

`master` をマージし、ゲームに不要な `Sandbox/` は自動で取り除きます。
コンフリクトが残ったときは表示されたファイルを直して `git add` → `git commit` してください。
`master` にまだマージしていない機能を先に使いたいときは、末尾に `-EngineBranch feature/〇〇` を付けるとそのブランチから取り込めます。

> Visual Studio のマージ機能や `git merge` を直接使わず、このスクリプトを使ってください。
> git は `Game/Game.vcxproj` や `Game/main.cpp` を「Sandbox のファイルを名前変更したもの」とみなすことがあり、
> 普通にマージするとエンジン側での Sandbox の変更がゲームのプロジェクトに混ざってしまいます
> （スクリプトは名前変更の検出を切ってマージします）。

## ゲーム制作中にエンジンの修正が必要になったら

1. ゲーム側の作業をコミット（または `git stash`）する
2. `master` から `feature/〇〇` を作ってエンジンを修正し、Sandbox で動作確認してコミットする
3. `feature/〇〇` を `master` へマージする
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
| 当たり判定 | `Engine::IsCollision(a, b)`（`Engine/Math/Collision.h`。球・AABB・OBB・線分など） |
| 曲線 | `Engine::Curve` の `GetPoint(t)`（`Engine/Math/Curve.h`） |
| デバッグ線 | `Engine::DebugDraw::DrawSphere(sphere, color);` など（`OnUpdate` で毎フレーム呼ぶ） |
| 時間 | 更新は毎秒60回に固定。秒単位の処理は `Engine::Time::GetDeltaTime()` |

## アセットと実行時のカレントディレクトリ

- ゲームのアセットは `Game/resources/` に置き、`"resources/xxx.png"` のように読み込みます
- VS から実行すると、カレントディレクトリはアプリのフォルダ（`Game/` や `Sandbox/`）になります
- エンジンのシェーダーは `Shaders/` → `../Shaders/` の順に探します
- 配布するときは exe と dll（`dxcompiler.dll` / `dxil.dll`）、`Shaders/`、`resources/` を同じフォルダに置きます

## 補足

- ブランチを切り替えても、git 管理外のファイル（`imgui.ini`・`logs/` など）はそのまま残ります。
  ゲーム制作ブランチに `Sandbox/` フォルダが残って見えても、中身が管理外のファイルだけなら問題ありません
- ビルド構成は3つで、ソリューションの構成はそれぞれ同じ名前のプロジェクト構成でビルドされます
  - `Debug` … ImGui・デバッグカメラあり、最適化なし。エンジンの中をデバッグするときや、ImGui で値を調整するときに使う
  - `Development` … ふだんのゲーム開発用。ImGui なし・assert 有効。ゲーム（`Game/`・`Sandbox/`）のコードは最適化なしでデバッガで追え、エンジン（`Engine.lib`）は最適化あり
  - `Release` … 提出・配布用。すべて最適化あり（プログラム全体の最適化も）、assert なし（`NDEBUG`）
