# コードの書き方（ゲーム側の約束）

ゲーム側（`Game/`）のコードは、ユーザー自身が頻繁に編集します。そのため **これまでのコードに近い書き方** をし、過去のコードに 2 通りの書き方があるときは、より良い方に揃えます。
基準にするのは最新で最も規模の大きい AL3（KamataEngine 製、タグ AL3_05_19_ex3 まで）の書き方です。昨年作（Novice 製）とは異なる点は AL3 側を採用します。観察の根拠は [knowledge/AL3_ex3.md](knowledge/AL3_ex3.md) の「コーディングスタイル」と [knowledge/GameExample.md](knowledge/GameExample.md) の §3。

2026-10-07 の決定: 所有には `std::unique_ptr` を使う ／ ラムダ式と constexpr は使わない ／ 調整値の保存は CSV。

## 命名

| 対象 | 書き方 | 例 |
| --- | --- | --- |
| クラス・struct | PascalCase の英語名詞。役割の接尾辞 `…Scene` `…Manager` `…Controller` `…Effect`、基底は `Base…` | `GameScene`, `BaseEnemy` |
| class のメンバ変数 | camelCase ＋ 末尾 `_` | `velocity_`, `isGrounded_` |
| struct のメンバ | camelCase（末尾 `_` なし） | `min`, `radius` |
| static の共有メンバ | `s` 接頭辞 ＋ 末尾 `_` | `sModel_` |
| 定数 | `k` 接頭辞 ＋ PascalCase。クラス内は `static inline const`、ローカルは `const`（constexpr は使わない） | `kJumpSpeed`, `kAttackDuration` |
| enum | `enum class`、列挙子は `k` ＋ PascalCase、要素数の番兵は `kNum…` | `enum class Phase { kIdle, kCharge, kNumPhase }` |
| 関数 | PascalCase、動詞始まり。`Initialize` ／ `Update` ／ `Draw`、`Get…` ／ `Set…` ／ `Is…` ／ `Has…` ／ `On…` ／ `Calc…` ／ `Find…` ／ `Load…` ／ `Reset` | `UpdateAttack()`, `IsAlive()` |
| bool | `is…` ／ `has…` ／ `on…`（昨年作の `…Flag` 形は使わない） | `isThrowing_` |
| ローカル・引数 | camelCase。数学の短い変数 `t`, `i`, `x` は可。出力引数は `out…` | `outSurfaceY` |
| ファイル | クラス名と同じ PascalCase の `.h` ／ `.cpp` を 1 組 | `Boomerang.h` ／ `Boomerang.cpp` |
| 言語 | 識別子は英語、コメント・`#pragma region` の見出し・assert の説明は日本語 | |

## ファイル構成

- ヘッダ: `#pragma once` → 自作ヘッダ（アルファベット順）→ 標準ヘッダ → 前方宣言 → クラス
- .cpp: 自分のヘッダ → 他の自作ヘッダ → 標準ヘッダ → `using namespace Engine;` → 無名名前空間（ファイル内の定数・ヘルパ）→ 関数定義
- `using namespace` はヘッダに書かない（ヘッダでは `Engine::Vector3` と書く）
- インクルードはリポジトリ直下基準: `#include "Engine/Input/Input.h"`、`#include "Game/Object/Player.h"`
- クラス内の並びは **public → protected → private**（AL3 の新しいクラス側の形に統一）。関数は ctor/dtor → Initialize → Update → Draw → getter/setter/判定 → private ヘルパ
- .cpp 側は「Initialize → Update → 状態別 Update → 補助 → Draw → 当たり判定」の機能順に並べ、`#pragma region 日本語見出し` で区切る
- 新しいクラスは `Game/Game.vcxproj` と `Game/Game.vcxproj.filters` の両方に登録する（Visual Studio の「追加 → 新しい項目」なら自動）

## クラスの形

- 既定値はクラス内初期化子で、実行時の状態は `Initialize()` で入れる。コンストラクタは基本的に空（または `= default`）
- 外部の依存（カメラ、モデル、他オブジェクト）はポインタで受け取ってメンバに保存。入力データは `const T&`、出力は `T&`
- 必須の依存は `assert(ptr)`、任意のものは `if (ptr)` で分岐
- 所有権: 生成した側が解放する。所有には `std::unique_ptr` ／ `std::make_unique` を使い、`new` ／ `delete` を直接書かない。借りるだけの相手は生ポインタ
- `Object3D` ／ `Sprite` は Initialize 後にコピーしない（配列で持つなら `std::vector` を reserve してから要素ごとに Initialize するか、unique_ptr で持つ）
- 固定数は `std::array` ／ 値メンバ、可変数・多態は `std::vector`
- 判定は bool を返す関数にまとめる（`IsGrounded()`, `IsHit(...)`）
- ラムダ式は使わない。共通化したい処理は名前付きの関数（メンバ関数か無名名前空間の関数）にする

## 状態と時間

- 状態は `enum class` ＋ `switch`。状態ごとに `UpdateIdle()` ／ `UpdateAttack()` のような小さな関数へ分ける（関数名は動詞先で統一。`BehaviorAttackInitialize` のような名詞先は使わない）
- 状態の切替は AL3 と同じく「切替要求 → 次フレーム頭で初期化関数」。切替時の初期化は `Start…()` の 1 関数にまとめる
- 時間はフレーム基準。秒で持ちたいタイマーは毎フレーム `1.0f / 60.0f` を足す（長さの定数は秒）。1 フレームあたりの移動量で速度を書いてよい
- 演出は正規化時間 `t = timer / duration` を 0〜1 に clamp してイージングに通す

## 定数と調整値

- マジックナンバーは `k` 定数に抽出し、行末コメントで意味を書く
- ゲームの手触りに関わる値（速度・時間・半径など）はクラス先頭にまとめ、ImGui で触れるようにする。保存・読込は CSV（調整項目の仕組みは AllowedKnowledge §4.5）
- 円周率は `std::numbers::pi_v<float>`。度→ラジアンは定数倍で書く
- `std::min` ／ `std::max` は Windows.h のマクロと衝突するので、比較式か `(std::min)(a, b)` で書く

## 書式

- インデントはタブ、波括弧は K&R（`if (...) {` 同じ行、`} else {`）。単文の `if` にも波括弧を付ける
- 浮動小数は `float` ＋ `f` 接尾辞（`0.5f`）
- 1 行はおおむね 120 文字まで。長い式は一時変数に分ける
- 「`// 説明` 1 行 → 数行のコード → 空行」のリズムで書く。関数間は 1 空行
- 使わなくなったコードはコメントアウトで残さず削除する（履歴は git にある）

## コメント

- クラスと public 関数には `/// <summary>…</summary>`（引数・戻り値は `<param>` `<returns>`）
- メンバ変数・private 関数・処理の説明は `//` の日本語 1 行コメント。理由や不具合対策の経緯も書く
- 連続する宣言の行末コメントは位置を揃える

## エンジンとの付き合い方

- `Engine/` は変更しない。足りない機能はゲーム側のクラスか関数で補う（AllowedKnowledge §5）
- ImGui を使うコードは `#ifdef USE_IMGUI` で囲み、シーンの `OnDrawImGui()` に書く。開発中だけのコードは `#ifndef NDEBUG`（`_DEBUG` は使わない）
- `/W4 /WX` なので未使用の変数・引数はビルドエラー。使わない引数は名前をコメントアウトする（`float /*deltaTime*/`）
- `Object3D` ／ `Sprite` は毎フレーム `Update()` → `Draw()`。スプライトは `OnDraw` の最後、半透明は不透明の後に奥から描く
- 音声ファイルの読込（`Audio::LoadWave`）はアプリで 1 回だけ（キャッシュが無いため）

## 昨年作から引き継がないもの

- `typedef struct` と全処理を 1 関数に書く形 → クラスに分ける
- `enum` の UPPER_SNAKE 列挙子 → `enum class` ＋ `kName`
- int をフラグに使う → `bool`
- 画面外座標 (-128, -128) で非アクティブを表す → `isActive_` フラグ
- `min` ／ `max` マクロでの int ／ float 混在 → 比較式
- スクリーン座標で直接計算する書き方 → ワールド座標で計算し、マウスだけスクリーン→ワールドに変換する
