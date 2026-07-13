# Verify DirectXGame (CG2)

Win32/DirectX12のGUIアプリ。サーフェスはゲームウィンドウ（キー入力＋描画結果）。

## Build

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" DirectXGame.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /verbosity:minimal /nologo
```

- Debug/Releaseとも /W4 警告=エラー。`/t:Rebuild`やCleanは使わない（DirectXTexのシェーダー.incが消えて9009で死ぬ。消えたら `git restore externals/DirectXTex/Shaders/Compiled/`）。
- ImGui（USE_IMGUI）はDebugのみ。

## Launch & drive

- 実行: `x64\Debug\CG2.exe` を**リポジトリルートをカレント**にして起動（resources/ が相対パス）。
- 入力はDirectInputで `DISCL_FOREGROUND`。**ウィンドウが本当に前面でないとキーが届かない**。
  - `SendKeys`単体では届かない。Altキーハック（keybd_event(0x12)押下→SetForegroundWindow→Alt解放）で前面化し、`GetForegroundWindow()`で検証してから送る。
  - キーは `keybd_event(0, scancode, KEYEVENTF_SCANCODE, 0)` でスキャンコード送信、約80ms押下維持（60fpsポーリングのため）。
- 実績のある駆動スクリプトの例: セッションのscratchpadに置いた`drive_app2.ps1`方式
  （起動→前面化→Snap→キー→Sleep 2s→Snap…→Stop-Process）。
- スクリーンショット: `GetWindowRect`＋`Graphics.CopyFromScreen`（先に`SetProcessDPIAware()`）。
- PowerShell 5.1でスクリプトを書くときは**ASCIIのみ**（BOMなしUTF-8の日本語コメントはパースエラーになる）。

## Key map / flows

- `Tab`(0x0F): 通常デモシーン切替 kGame ⇔ kAxis（立体視デモ中は無効）
- `T`(0x14): 通常シーン ⇔ 立体視デモ（戻り先は直前にいた通常シーン）
- `Space`: サウンド再生 / `Enter`: デバッグカメラ / `F11`: ボーダレス全画面
- シーンの判別はImGuiの"Frustum Culling"ウィンドウの項目名と"3D Objects"の内容で行う
  （kGame=Triangle/Obj/Sprite、kAxis=Obj/Sphere、立体視デモ=Cubes: N）。
- スプライトのスケール検証は `MoveWindow` でリサイズ→スクショで縦横比を実測。
