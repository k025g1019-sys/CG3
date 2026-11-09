# CG3

[![DebugBuild](https://github.com/k025g1019-sys/CG3/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/k025g1019-sys/CG3/actions/workflows/DebugBuild.yml)
[![DevelopmentBuild](https://github.com/k025g1019-sys/CG3/actions/workflows/DevelopmentBuild.yml/badge.svg)](https://github.com/k025g1019-sys/CG3/actions/workflows/DevelopmentBuild.yml)
[![ReleaseBuild](https://github.com/k025g1019-sys/CG3/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/k025g1019-sys/CG3/actions/workflows/ReleaseBuild.yml)

DirectX 12 で作っている自作エンジン（静的ライブラリ）と、それを使うアプリのリポジトリです。
`main` にはエンジンと、エンジンの動作確認用アプリ（Sandbox）が入っています。

## 必要なもの

- Windows 10 / 11（x64）
- Visual Studio 2026（「C++ によるデスクトップ開発」。プラットフォームツールセット v145）

## ビルドと実行

1. Visual Studio 2026 で `project/CG3.slnx` を開きます
2. ソリューション構成（Debug / Development / Release）を選びます
3. F5 でビルドして実行します

ビルドで作られるファイルは、プロジェクトフォルダの外の `generated/` に出ます（git 管理外）。

| フォルダ | 中身 |
|---|---|
| `generated/outputs/<構成>/` | exe・lib・dll |
| `generated/obj/<プロジェクト>/<構成>/` | 中間ファイル |

## ビルド構成

| 構成 | 用途 |
|---|---|
| Debug | エンジンも含めて最適化なし。ImGui・デバッグカメラあり。エンジンの中をデバッグするとき |
| Development | ふだんの開発用。ImGui・デバッグカメラあり、assert 有効。アプリのコードだけ最適化なし |
| Release | 提出・配布用。ImGui・デバッグカメラなし、すべて最適化あり |

## フォルダ構成

| フォルダ | 中身 |
|---|---|
| `project/Engine/` | エンジン本体（`Engine.vcxproj`、共通ビルド設定 `Engine.props`） |
| `project/Engine/Shaders/` | エンジンのシェーダー（HLSL。実行時にコンパイルする） |
| `project/externals/` | 外部ライブラリ（DirectXTex・imgui） |
| `project/Sandbox/` | エンジンの動作確認用アプリ（`resources/` に実行時に読み込むファイル） |
| `docs/` | 手順書 |
| `tools/` | 補助ツール |

エンジン制作とゲーム制作の進め方は [docs/EngineGameWorkflow.md](docs/EngineGameWorkflow.md) にまとめています。

## 自動ビルド（GitHub Actions）

`main` と `feature/〇〇` へのプッシュ、`main` へのプルリクエストのたびに、
Debug・Development・Release の3つの構成をそれぞれ GitHub 上でビルドします（`.github/workflows/`）。

- 上のバッジは `main` の最新のビルド結果です。緑の「passing」ならビルド成功、赤の「failing」なら失敗です
- バッジをクリックすると、そのワークフローの実行結果の一覧が開きます
- リポジトリの「Actions」タブで各ワークフローを選び、「Run workflow」から手動で実行することもできます
