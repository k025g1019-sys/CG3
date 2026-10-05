# Verify DirectXGame (CG3)

Win32/DirectX12のGUIアプリ。サーフェスはゲームウィンドウ（キー入力＋描画結果）。

構成: `Engine/Engine.vcxproj`（静的ライブラリ）＋アプリ1つ。アプリはブランチで変わる。
- エンジン制作ブランチ: `Sandbox/Sandbox.vcxproj`（エンジン動作確認用デモ）→ `x64\<構成>\Sandbox.exe`
- ゲーム制作ブランチ: `Game/Game.vcxproj`（ゲーム本体）→ `x64\<構成>\Game.exe`

## Build

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" CG3.slnx /p:Configuration=Debug /p:Platform=x64 /m /verbosity:minimal /nologo
```

- 構成は Debug / Development / Release の3つで、ソリューションの各構成は同じ名前のプロジェクト構成でビルドされる。
  Development = ImGuiあり（USE_IMGUI）・assert有効（NDEBUGなし）・アプリ（Sandbox/Game）は /Od、Engine.lib は /O2 + /GL（アプリのリンク時に /LTCG で再リンクされる）。
  Release = NDEBUG・全プロジェクト /O2 + /GL・LTCG。エンジン全体に効く変更は3構成ともビルドして確かめる。
- 全構成 /W4 警告=エラー。`/t:Rebuild`やCleanは使わない（DirectXTexのシェーダー.incが消えて9009で死ぬ。消えたら `git restore externals/DirectXTex/Shaders/Compiled/`）。
- ImGui（USE_IMGUI）は Debug と Development にあり、Release にはない。USE_IMGUI はエンジンのクラスの中身を変えるので、エンジンとアプリで必ずそろえる。

## Launch & drive

- 実行: アプリのフォルダ（`Sandbox/` か `Game/`）を**カレント**にして exe を起動する（resources/ はそのフォルダからの相対パス。
  エンジンのシェーダーは `Shaders/` → `../Shaders/` の順に探す）。
- 入力はDirectInputで `DISCL_FOREGROUND`。**ウィンドウが本当に前面でないとキーが届かない**。
  - `SendKeys`単体では届かない。Altキーハック（keybd_event(0x12)押下→SetForegroundWindow→Alt解放）で前面化し、`GetForegroundWindow()`で検証してから送る。
  - キーは `keybd_event(0, scancode, KEYEVENTF_SCANCODE, 0)` でスキャンコード送信、約80ms押下維持（60fpsポーリングのため）。
- 実績のある駆動スクリプトの例: セッションのscratchpadに置いた`drive_app2.ps1`方式
  （起動→前面化→Snap→キー→Sleep 2s→Snap…→Stop-Process）。
- スクリーンショット: `GetWindowRect`＋`Graphics.CopyFromScreen`（先に`SetProcessDPIAware()`）。
- PowerShell 5.1でスクリプトを書くときは**ASCIIのみ**（BOMなしUTF-8の日本語コメントはパースエラーになる）。

## Key map / flows（Sandbox）

- `Tab`(0x0F): 通常デモシーン切替 kGame ⇔ kAxis（立体視デモ中は無効）
- `T`(0x14): 通常シーン ⇔ 立体視デモ（戻り先は直前にいた通常シーン）
- `Space`: サウンド再生 / `Enter`: デバッグカメラ / `F11`: ボーダレス全画面
- シーンの判別はImGuiの"Frustum Culling"ウィンドウの項目名と"3D Objects"の内容で行う
  （kGame=Triangle/Obj/Sprite、kAxis=Obj/Sphere、立体視デモ=Cubes: N）。
- スプライトのスケール検証は `MoveWindow` でリサイズ→スクショで縦横比を実測。
