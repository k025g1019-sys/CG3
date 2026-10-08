# PG3 リポジトリ 知識項目インベントリ

対象: `C:\Users\s-dai\source\repos\PG3`（master、HEAD = 43f8f0b = タグ PG3_02_02、作業ツリーはクリーン）。
章ごとに main.cpp が上書きされているため、根拠は **`main.cpp:行番号 (タグ名)`** の形で示す。

## 調査範囲（実際に全文を読んだもの）
- 0e23887 `main.cpp`（9 行、「ツ」を出力する初期雛形。00_01 の直前コミット）
- PG3_00_01 `main.cpp`（9 行）
- PG3_01_01 `main.cpp`（70 行）
- PG3_01_02 `main.cpp`（19 行）
- PG3_02_01 `main.cpp`（33 行）
- PG3_02_02 `main.cpp`（63 行）＋ `answer02-02.md`（7 行）
- `PG3.vcxproj`（全タグ）、`.editorconfig`、`PG3.slnx`、`PG3.vcxproj.filters`、タグ間 diff、コミットメッセージ
- `git ls-tree -r --name-only <tag>` を全タグで確認: 学生のソースは **main.cpp のみ**（ヘッダ・他の .cpp は存在しない）。無視ディレクトリ（x64/, PG3/, .vs/）にもソース無し。

---

## A. C++ 言語機能・標準ライブラリ

### A-1. 基本構文
- **`int main()` と `return 0;`** — 引数なしの main、終了コード 0 を返す。全タグ共通 — 根拠: main.cpp:3,8 (PG3_00_01), :6,69 (PG3_01_01), :8,18 (PG3_01_02), :13,32 (PG3_02_01), :34,62 (PG3_02_02)
- **`#include <...>` による標準ヘッダの取り込み** — `<iostream>`(全タグ)、`<stdio.h>`(01_01, 01_02)、`<list>`(01_01, 01_02)、`<algorithm>`(01_02)、`<vector>`(01_02)、`<cstdlib>`(02_02) — 根拠: main.cpp:1 (PG3_00_01), :1-3 (PG3_01_01), :1-5 (PG3_01_02), :1 (PG3_02_01), :1-2 (PG3_02_02)
- **`using namespace std;`（グローバル）** — `std::` を省略するため冒頭に記述。独自 namespace の定義は無い — 根拠: main.cpp:4 (PG3_01_01), :6 (PG3_01_02), :2 (PG3_02_01), :3 (PG3_02_02)
- **行コメント `//`** — 処理の目的を日本語で記述（「西日暮里を追加」「n時間目の再帰的な賃金を求める」など） — 根拠: main.cpp:4 (PG3_00_01), :7,45,57 (PG3_01_01), :9 (PG3_01_02), :4,14,17,22,27 (PG3_02_01), :5,16,26,40,53 (PG3_02_02)
- **変数の宣言と同時初期化** — `int intA = 5;`、`int hour = 1;`、`int normalTotal = normalYen * hour;` — 根拠: main.cpp:18-19,23-24,28-29 (PG3_02_01), :37,42-43 (PG3_02_02)
- **基本型 int / float / double** — 同じ関数テンプレートを 3 種類の型で呼び分ける — 根拠: main.cpp:18,23,28 (PG3_02_01)
- **float リテラル接尾辞 `f`・double リテラル** — `3.2f`, `2.2f`, `16.41`, `8.31` — 根拠: main.cpp:23-24,28-29 (PG3_02_01)
- **`const` ローカル定数** — `const int normalYen = 1226;`（時給の固定値） — 根拠: main.cpp:38 (PG3_02_02)
- **`char` 配列を文字列リテラルで初期化** — `char str[] = "タ";`（UTF-8 多バイト文字を C 文字列として保持） — 根拠: main.cpp:6 (PG3_00_01)
- **波括弧による初期化子リスト `= { ... }`** — list / vector を要素列挙で初期化（末尾カンマありの形も使用） — 根拠: main.cpp:10-39 (PG3_01_01), :12 (PG3_01_02)
- **日本語を含む文字列リテラル（ソース UTF-8）** — 日本語をそのままリテラルに書いて出力 — 根拠: main.cpp:6 (PG3_00_01), :9,11,13,45-49,55 (PG3_02_02)
- **エスケープシーケンス `\n`** — printf / cout の両方で改行に使用 — 根拠: main.cpp:40,42,52,64 (PG3_01_01)
- **`bool` リテラル `true`** — `while (true)` の無限ループ条件としてのみ使用（bool 変数・bool 関数は無い） — 根拠: main.cpp:41 (PG3_02_02)

### A-2. 制御構文
- **`if` / `else`（波括弧なしの単文）** — テンプレート Min の 2 分岐 — 根拠: main.cpp:7-10 (PG3_02_01)
- **`if` / `else if` / `else`（3 分岐）** — 大きい・小さい・等しいで出力を切り替え — 根拠: main.cpp:8-13 (PG3_02_02)
- **`if` 単独（波括弧あり）** — 要素一致時の挿入、終了条件判定、再帰の基底条件 — 根拠: main.cpp:47-50,59-62 (PG3_01_01), :18-21,28-29,54-57 (PG3_02_02)
- **添字カウンタ式 `for` ループ** — `for (int i = 0; i < name.size(); i++)` で vector を走査 — 根拠: main.cpp:14-16 (PG3_01_02)
- **イテレータ式 `for` ループ** — `for (auto itr = X.begin(); itr != X.end(); ++itr)` で list を走査（範囲 for は未使用） — 根拠: main.cpp:41,46,53,58,65 (PG3_01_01)
- **`while (true)` + `break`** — 条件成立（再帰賃金 > 一般賃金）まで hour を 1 ずつ増やし、成立したら脱出 — 根拠: main.cpp:41-60, 56 (PG3_02_02)
- **比較演算子 `<` `>` `<=` `==` `!=`** — 値比較、再帰の基底条件、イテレータ終端判定、ポインタ比較 — 根拠: main.cpp:41,47 (PG3_01_01), :7 (PG3_02_01), :8,10,18,20,28,54 (PG3_02_02)
- **算術演算子 `*` `-` `+`、後置 `++`、前置 `++`** — `ResultRecursionYen(hour - 1) * 2 - 50`、`normalYen * hour`、`hour++`、`++itr` — 根拠: main.cpp:41,49 (PG3_01_01), :23,31,42,59 (PG3_02_02)

### A-3. 関数
- **自作関数の定義（main より前に定義、プロトタイプ宣言なし）** — Min / ComparisonResult / ResultRecursionYen / TotalRecursionYen の 4 つ — 根拠: main.cpp:5-11 (PG3_02_01), :6-32 (PG3_02_02)
- **値渡し引数と戻り値（int / T / void）** — すべて値渡し。参照 `&` 引数・ポインタ引数・関数オーバーロードは未使用 — 根拠: main.cpp:6 (PG3_02_01), :7,17,27 (PG3_02_02)
- **関数テンプレート `template <typename T>`** — 戻り値 T の `T Min(T a, T b)`、戻り値 void の `void ComparisonResult(T normal, T recursion)` — 根拠: main.cpp:5-6 (PG3_02_01), :6-7 (PG3_02_02)
- **テンプレート引数の暗黙推論** — `Min(intA, intB)` のように `<int>` を書かずに呼び、int / float / double で実体化。02_02 では int のみ — 根拠: main.cpp:20,25,30 (PG3_02_01), :51 (PG3_02_02)
- **再帰関数（自己再帰・複数の基底条件）** — `ResultRecursionYen(hour)`: `hour <= 0 → 0`、`hour == 1 → 100`、それ以外 `f(hour-1) * 2 - 50` — 根拠: main.cpp:17-24 (PG3_02_02)
- **再帰による総和** — `TotalRecursionYen(h) = ResultRecursionYen(h) + TotalRecursionYen(h-1)`（基底 `h <= 0 → 0`） — 根拠: main.cpp:27-32 (PG3_02_02)

### A-4. 標準ライブラリ（コンテナ・アルゴリズム）
- **`std::list<const char*>`（双方向連結リスト）** — 初期化子リストで山手線 28 駅を初期化、`begin()` / `end()`、`insert(itr, 値)`（指定位置の前に挿入し、新要素のイテレータを返す）で途中挿入 — 根拠: main.cpp:3,10-39,41,48,60 (PG3_01_01)
- **`std::list<...>::iterator` の明示型とイテレータ操作** — 明示型指定、`*itr` で参照外し、`++itr`、`insert` の戻り値を `itr` に再代入してから `++itr` で挿入要素と一致要素を飛ばして走査継続 — 根拠: main.cpp:46-50,58-62 (PG3_01_01)
- **`auto`（イテレータ型の推論）** — `auto itr = Stations.begin()` のみに使用（他の用途は無い） — 根拠: main.cpp:41,53,65 (PG3_01_01)
- **`std::vector<std::string>`** — 初期化子リストで 50 件の学籍メールアドレスを初期化、`size()`、`operator[]`、`begin()` / `end()`。push_back / erase 等は未使用 — 根拠: main.cpp:5,12-15 (PG3_01_02)
- **`std::string`** — vector の要素型として使用（`<string>` は未 include、`<iostream>` 経由で間接利用） — 根拠: main.cpp:12 (PG3_01_02)
- **`std::sort`（`<algorithm>`）** — `sort(name.begin(), name.end())` で既定の `operator<` による辞書順昇順ソート（比較関数・ラムダ無し） — 根拠: main.cpp:4,13 (PG3_01_02)
- **`const char*`（文字列リテラルへのポインタ）** — list の要素型として C 文字列を保持するのが唯一のポインタ使用。アドレス演算子 `&`、ポインタ演算、`new` / `delete`、`nullptr` は全タグに無い — 根拠: main.cpp:10,46,58 (PG3_01_01)

### A-5. 標準入出力
- **`std::cout` と `<<` の連鎖** — 数値と文字列を連結して出力、複数行にまたがる連鎖も使用 — 根拠: main.cpp:42 (PG3_01_01), :15 (PG3_01_02), :20,25,30 (PG3_02_01), :9-13,45-49,55 (PG3_02_02)
- **`std::endl` と `"\n"` の両方で改行** — 01_01 は `"\n"`、01_02 以降は `endl` — 根拠: main.cpp:42 (PG3_01_01), :15 (PG3_01_02), :20 (PG3_02_01), :9 (PG3_02_02)
- **`printf`（書式 `%s`・固定文字列）** — `printf("%s", str)`、`printf("age 1970\n")`。cout と混在使用 — 根拠: main.cpp:7 (PG3_00_01), :40,52,64 (PG3_01_01)
- **`system("chcp 65001 > nul")`（`<cstdlib>` / C ランタイム）** — Windows コンソールの文字コードを UTF-8 に切り替え（`> nul` で出力抑制）。全タグの main 冒頭に存在、02_02 で初めて `<cstdlib>` を明示 include — 根拠: main.cpp:5 (PG3_00_01), :8 (PG3_01_01), :10 (PG3_01_02), :15 (PG3_02_01), :2,35 (PG3_02_02)

### A-6. 全タグに出現しない機能（このソースからは「学んだ」と言えない）
- **class / struct、継承、仮想関数・純粋仮想、コンストラクタ / デストラクタ、演算子オーバーロード、クラステンプレート、関数オーバーロード、ラムダ式、範囲 for、enum / enum class、constexpr、static、参照 `&`、new / delete、unique_ptr / shared_ptr / make_unique、std::function、std::map / unordered_map / array / pair、`<cmath>` / `<random>` / `<fstream>` / `<sstream>` / `<filesystem>` / `<cassert>`、例外（try / catch / throw）、`static_cast` 等のキャスト、`#pragma once` / ヘッダ分割、独自 namespace、switch** — 全タグの main.cpp を通読して不在を確認 — 根拠: PG3_00_01〜PG3_02_02 の main.cpp 全文

---

## B. 数学・物理・当たり判定
- **該当なし（ベクトル / 行列 / 変換 / 補間 / 曲線 / 物理 / 当たり判定 / 四元数は一切なし）** — 唯一の数学的内容は整数の漸化式 `f(1)=100, f(h)=2·f(h-1)-50` とその累積和、および `normalYen * hour` との大小比較 — 根拠: main.cpp:17-32,42-43 (PG3_02_02)
- **汎用 Min（2 値の大小比較）** — 小さい方を返す自作関数テンプレート（`std::min` は不使用） — 根拠: main.cpp:5-11 (PG3_02_01)

---

## C. ゲームの構造・実装パターン
- **ゲーム固有の構造は無し** — シーン遷移、状態管理、入力処理、カメラ、マップチップ、エフェクト、タイマー、ImGui、乱数、コードからのファイル読み書きはいずれも全タグに存在しない — 根拠: PG3_00_01〜PG3_02_02 の main.cpp 全文
- **（応用可能）コンテナへの途中挿入パターン** — list を走査し、一致要素を見つけたらその直前に `insert`、戻り値でイテレータを更新して走査を続ける（西日暮里→田端の前、高輪ゲートウェイ→田町の前） — 根拠: main.cpp:46-51,58-63 (PG3_01_01)
- **（応用可能）一覧のソートと列挙表示** — vector に溜めた文字列を `sort` → 添字ループで全件出力 — 根拠: main.cpp:13-16 (PG3_01_02)
- **（応用可能）条件成立まで反復するシミュレーションループ** — `while (true)` で時間を 1 ずつ進め、毎ステップの状態を出力し、条件成立で `break` — 根拠: main.cpp:41-60 (PG3_02_02)
- **（応用可能）コンソール出力による結果確認（デバッグ表示に相当）** — 各ステップの合計値と比較結果を文字で表示 — 根拠: main.cpp:45-51 (PG3_02_02)
- **課題回答を Markdown に手書きで記録** — `answer02-02.md` に 7 時間（一般 8582 円 / 再帰 6700 円）、8 時間（一般 9808 円 / 再帰 13150 円）→「8 時間で再帰的な賃金体系のほうが儲かる」と記録。プログラムからのファイル出力ではない — 根拠: answer02-02.md:1-7 (PG3_02_02)

---

## D. 使用しているフレームワーク / エンジン API
- **標準入出力のみのコンソールアプリ** — Novice / KamataEngine / DirectX / ImGui / Win32 API は一切使用していない。vcxproj の SubSystem は Console、include は標準ヘッダのみ — 根拠: 全タグ main.cpp の include 行、PG3.vcxproj（`<SubSystem>Console</SubSystem>`）
- **`printf`（`<stdio.h>` / `<iostream>` 経由）** — 文字列出力 — 根拠: main.cpp:7 (PG3_00_01), :40,52,64 (PG3_01_01)
- **`std::cout` / `std::endl`（`<iostream>`）** — 値・文字列の出力 — 根拠: main.cpp:42 (PG3_01_01), :15 (PG3_01_02), :20,25,30 (PG3_02_01), :9-13,45-49,55 (PG3_02_02)
- **`system()`（C ランタイム `<cstdlib>`）** — OS コマンド `chcp 65001` の実行のみ — 根拠: main.cpp:5 (PG3_00_01) ほか全タグ
- **`std::sort`（`<algorithm>`）、`std::list`、`std::vector`、`std::string`** — 標準ライブラリのコンテナ・アルゴリズム（詳細は A-4） — 根拠: main.cpp:10 (PG3_01_01), :12-13 (PG3_01_02)

---

## E. 判断に迷うもの・備考
- **雛形由来のコード** — 初期コミット 0e23887 の main.cpp（`system("chcp 65001 > nul")`、`char str[] = "ツ"`、`printf("%s", str)`）は課題 00_01 の雛形で、学生が書いたのは「ツ」→「タ」の 1 文字変更のみ。`system("chcp ...")` 行は以後の全タグにそのまま引き継がれている — 根拠: `git diff 0e23887 PG3_00_01`（main.cpp:6 のみ変更）
- **「双方向リスト」は自作ではなく `std::list`** — コミットメッセージは「双方向リストを作成して」だが、Node 構造体やポインタによる自作連結リストは無く、標準の `std::list` を使っている。「連結リストの自作（prev/next ポインタ）」は学んだとは言えない — 根拠: main.cpp:10 (PG3_01_01)
- **`*itr == "Tabata"` は C 文字列のポインタ比較** — `const char*` 同士を `==` で比べており、内容比較（`strcmp` / `std::string`）ではない。同一リテラルがコンパイラにまとめられる（文字列プーリング）前提で期待どおりに動いている書き方で、設定次第で一致しない可能性がある。「文字列比較を習得した」とは判断しづらい — 根拠: main.cpp:47,59 (PG3_01_01)
- **`std::string` を `<string>` 無しで使用** — `<iostream>` 経由の間接 include に依存 — 根拠: main.cpp:1,12 (PG3_01_02)
- **不要 include の残存** — 01_02 で `<list>`・`<stdio.h>` を include しているが未使用（01_01 からの残り） — 根拠: main.cpp:2-3 (PG3_01_02)
- **signed / unsigned 比較** — `int i < name.size()`（size_t）は警告 C4018 の対象になりうる書き方 — 根拠: main.cpp:14 (PG3_01_02)
- **printf と cout の混在** — 01_01 では見出しを printf、要素を cout で出力 — 根拠: main.cpp:40-42 (PG3_01_01)
- **`TotalRecursionYen` は二重再帰（O(n²)）** — 各時間の賃金を毎回再帰で計算し直す。課題規模（8 時間）では問題なし。answer02-02.md の数値は、この漸化式を手計算した値（各時間 100,150,250,450,850,1650,3250,6450 → 累積 100,250,500,950,1800,3450,6700,13150、一般 1226×7=8582 / 1226×8=9808）と一致する — 根拠: main.cpp:27-32,38 (PG3_02_02), answer02-02.md:3-6
- **関数テンプレートは同型 2 引数のみ** — `Min(int,int)` 等、異なる型を混ぜた呼び出し・明示的な型指定 `Min<int>(...)`・特殊化・クラステンプレートは無い — 根拠: main.cpp:20,25,30 (PG3_02_01), :51 (PG3_02_02)
- **日本語出力のためのビルド設定** — 02_02 で `PG3.vcxproj` の全 4 構成に `/utf-8` を追加（ソースの日本語リテラルを UTF-8 として扱う）。`chcp 65001` と組み合わせて日本語を正しく表示。`.editorconfig` は `[*.{cpp,h}] charset = utf-8` — 根拠: `git diff PG3_02_01 PG3_02_02 -- PG3.vcxproj`（AdditionalOptions /utf-8 ×4）、.editorconfig:1-2
- **開発環境** — Visual Studio（PlatformToolset v145、`.slnx` 形式のソリューション）、C++20（`<LanguageStandard>stdcpp20`、全タグ共通）、ConformanceMode、警告レベル Level3、SDLCheck、Win32 / x64 × Debug / Release。言語規格は C++20 だが、使用機能は C++11 範囲（auto、初期化子リスト、list / vector / sort） — 根拠: PG3.vcxproj:32-134（全タグで stdcpp20 を確認）
- **単一ファイル構成** — 全タグで学生のソースは main.cpp のみ。ヘッダ分割・複数 .cpp・クラス設計は無し — 根拠: `git ls-tree -r --name-only <tag>`（全 5 タグ）
- **タグ運用** — 課題ごとに main.cpp を上書きして `PG3_章_課題` 形式のタグを打つ運用。HEAD（master）には 02_02 の内容しか残っていない — 根拠: `git tag -l`、`git log --oneline`
- **個人情報** — 01_02 の vector には同級生の学籍メールアドレス 50 件が実データとして埋め込まれている（本レポートには転記しない） — 根拠: main.cpp:12 (PG3_01_02)
- **インデントの揺れ** — 00_01〜01_02 はタブ、02_01 は 4 スペース、02_02 はタブ（知識項目ではないが、雛形やコピー元の違いを示唆） — 根拠: 各タグ main.cpp

---

## このソースで学んだと言える主要トピック（タグ順）
1. **PG3_00_01** — コンソールアプリの最小構成（`#include <iostream>`、`int main`、`printf`、`return 0`）と、`system("chcp 65001")` による UTF-8 コンソール設定。学生の作業は雛形の 1 文字変更のみ。
2. **PG3_01_01** — `std::list<const char*>` による双方向連結リストの利用: 初期化子リスト、`begin()/end()` とイテレータ（明示型と `auto`）、`*itr` / `++itr`、`insert()` による途中挿入と戻り値イテレータの扱い、`using namespace std`、`cout` / `printf` 出力。
3. **PG3_01_02** — `std::vector<std::string>` と `<algorithm>` の `std::sort` による文字列の辞書順ソート、添字 `for` と `size()` / `operator[]` による列挙、`endl`。
4. **PG3_02_01** — 関数テンプレート `template <typename T>` の定義と、暗黙の型推論による呼び出し（int / float / double）、`if / else` による汎用 Min。
5. **PG3_02_02** — 再帰関数（複数の基底条件 + 再帰呼び出し、漸化式 `f(h)=2f(h-1)-50`、再帰による総和）、void 戻りの関数テンプレートによる 3 分岐比較、`const` 定数、`while (true)` + `break` のシミュレーションループ、`<cstdlib>` の明示 include、`/utf-8` + 日本語リテラル出力、Markdown での回答記録。
6. **全体を通じて** — 制御構文（if / else if / for / while / break）、基本型と算術・比較演算子、関数の定義と値渡し、単一ファイルのコンソールプログラム。クラス、ポインタ操作、動的確保、スマートポインタ、ラムダ、範囲 for、数学・物理・当たり判定、ゲームフレームワークは未登場。
