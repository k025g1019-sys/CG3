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

どちらのブランチでも `CG2.slnx` を開いて F5 で実行します。
エンジン制作ブランチでは `Sandbox`、ゲーム制作ブランチでは `Game` が起動します。

## ブランチ

- `CG3_develop` … エンジン制作（Engine / Shaders / Sandbox などを編集する）
- `AL4_develop` … ゲーム制作（`Game/` だけを編集する）

変更の取り込みは **エンジン → ゲーム の一方通行** です。ゲームのブランチをエンジンのブランチへマージしないでください。

## 最初に1回だけ

編集範囲のチェック（pre-commit フック）を有効にします。

```bash
git config core.hooksPath .githooks
```

- ゲーム制作ブランチで `Game/` 以外のファイルをコミットしようとすると止まります
- エンジン制作ブランチで `Game/` のファイルをコミットしようとすると止まります
- どうしても必要なときは `git commit --no-verify` でチェックを飛ばせます

## エンジンの変更をゲームに取り込む

ゲーム制作ブランチ（`AL4_develop`）で次を実行します。

```bash
powershell -ExecutionPolicy Bypass -File tools/MergeEngine.ps1
```

エンジンのブランチをマージし、ゲームに不要な `Sandbox/` は自動で取り除きます。
コンフリクトが残ったときは表示されたファイルを直して `git add` → `git commit` してください。

## ゲーム制作中にエンジンの修正が必要になったら

1. ゲーム側の作業をコミット（または `git stash`）する
2. `CG3_develop` に切り替えてエンジンを修正し、Sandbox で動作確認してコミットする
3. `AL4_develop` に戻り、`tools/MergeEngine.ps1` で取り込む

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
- ソリューション構成の `Release` はプロジェクトの `Development` 構成（ImGuiなし・最適化なし・assert有効）でビルドされます
