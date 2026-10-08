# 使える知識の一覧（AllowedKnowledge）

AL4 のゲーム制作は「授業で習得した知識のみを使う」制約で進めます。この文書はその判定基準です。
ゲーム側（`Game/`）のコードを書くときは、ここに載っている知識だけを使います。載っていないもの・△ のものは、使う前に止めて確認し、結果をここへ追記します。

- 作成: 2026-10-07。6 つの出典を全文読んで棚卸しし、同日の判断（§8）を反映済み
- 根拠の詳細（ファイル名:行番号つきの一覧）は [knowledge/](knowledge/) 配下の出典別レポートにあります
- 記号: **◎** 複数の出典にあり、そのまま使える ／ **○** 出典は 1 つ（または途中の章だけ）だが使える ／ **△** 要確認（§8 で判断を仰ぐ） ／ **✕** 使わない（どの出典にも無い、または使わないと決めたもの）
- 新しい課題を終えたら §1 と該当する節に追記し、§10 に日付を残します

## 1. 出典一覧

| 出典 | 場所 | 使える範囲 | 備考 |
| --- | --- | --- | --- |
| PG3 | `C:\Users\s-dai\source\repos\PG3`（git、タグ PG3_00_01〜PG3_02_02） | 全部 | コンソール課題。章ごとに main.cpp が上書きされているのでタグ単位で見る |
| MT3・MT4 | `C:\Users\s-dai\source\repos\MT4`（git、タグ MT3_00_01〜MT3_04_04 と MT4_00_01。HEAD が全章の累積） | 全部 | Novice 上の 3D 数学・物理・当たり判定。隣の `MT3_00_01`〜`MT3_01_02` フォルダは同じ内容の古いスナップショット |
| AL2_01 ／ AL2_02 ／ PG2_Test2 | `C:\Users\s-dai\Downloads\` の各フォルダ | 全部 | Novice 製の過去作（2D アクション ／ ステージテーブル ／ 3D 表示シューティング） |
| 昨年作（Game_example） | `C:\Users\s-dai\Downloads\Game_example\main.cpp` | 全部 | Novice 製「黄金のふきディフェンス！」。今回の構想の参考元 |
| AL3 | `C:\Users\s-dai\source\repos\AL3`（git）。知識の境界はタグ **AL3_05_19_ex3**。worktree: `C:\Users\s-dai\source\repos\AL3_ex3` | タグまで | KamataEngine 製。エンジン API に依存する部分は自作エンジン向けに書き換えて使う（§6）。タグ以降（完成版 Ver.1.0）は参考のみ。リソースは持ち込まない |
| 自作エンジン | `Engine/`（API 一覧は `Engine/Docs/EngineReference.html`） | 公開 API 全部 | CG2・CG3 で製作。制作中は変更しない |

出典の略記: PG3 ／ MT3 ／ MT4 ／ AL2（AL2_01）／ AL2b（AL2_02）／ PG2 ／ 昨年作 ／ AL3 ／ Eng（自作エンジン）

## 2. C++ 言語機能・標準ライブラリ

### 2.1 クラスと設計

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| class ／ struct、public・protected・private | ◎ | MT3, AL2, PG2, AL3 | データの束は struct、振る舞いを持つものは class（AL3 の使い分け） |
| コンストラクタ／デストラクタ、メンバ初期化子リスト | ◎ | MT3, AL2, PG2 | AL3 は「クラス内初期化子＋Initialize()」方式でコンストラクタは空 |
| クラス内初期化子（`int x_ = 0;`） | ◎ | MT3, PG2, AL2, AL3 | |
| `= default` ／ `= delete` | ◎ | MT3, PG2, AL3 | AL3 ではコピー禁止のシングルトンに使用 |
| 継承、virtual、純粋仮想、override、仮想デストラクタ | ◎ | MT3, AL2, PG2, AL3 | 基底 Scene ／ BaseEnemy ／ BaseEffect など |
| 基底ポインタのコンテナによる多態（`std::vector<Base*>`） | ◎ | PG2, AL3 | |
| 基底の実装を明示的に呼ぶ（`Base::Update()`） | ○ | AL3 | |
| static メンバ変数・static メンバ関数 | ◎ | PG2, AL3 | PG2 の敵マネージャ、AL3 の共有モデル |
| `static inline` データメンバ | ○ | AL3 | 調整項目の置き場 |
| 関数内 static 変数 | ◎ | PG2, AL3 | |
| シングルトン（関数内 static ＋ コピー禁止） | ○ | AL3 | GlobalVariables |
| Getter ／ Setter（ヘッダ内 1 行、const 付き） | ◎ | 全部 | |
| bool を返す判定関数（`IsX()` ／ `HasX()`） | ◎ | MT3, AL2, AL3 | 学校の OOP ルーブリックの項目でもある |
| 前方宣言 | ◎ | MT3, AL2, PG2, AL3 | |
| 無名名前空間 | ◎ | MT3, AL3 | ファイル内だけの定数・ヘルパ |
| `namespace` の定義 ／ `using namespace`（.cpp のみ）／ `using` 型エイリアス | ○ | AL3（PG3 は `using namespace std`） | ヘッダには書かない |
| inline 関数 | ◎ | MT3, AL3 | |
| 関数オーバーロード | ◎ | MT3, PG2, AL3 | |
| 関数テンプレート | ◎ | PG3, MT3 | |
| クラステンプレート | ✕ | — | |
| 演算子オーバーロード（+ - * / += -= *= /= 単項 - []） | ○ | MT3（MT3_03_02） | エンジンの Vector3 演算子を使うだけなら不要 |
| ラムダ式（`[&]`、auto 引数） | ✕ | MT3（3 箇所） | **決定: 使わない**（§8）。共通化は名前付き関数で |
| 構造体のネスト、クラス内 enum ／ struct | ◎ | 昨年作, AL3 | |
| 所有ポインタ ＋ new ／ delete（生成した側が delete） | ◎ | AL2, PG2, AL3, MT3 | 新作では下の unique_ptr を使う |
| `new[]` ／ `delete[]`、二重ポインタ | ○ | AL2 | 今は vector で足りる |
| std::unique_ptr ／ make_unique ／ std::move | ◎ | MT3（SceneManager、MT3_02_04〜） | **決定: 所有に使う**（§8）。エンジン方針（unique_ptr 優先）とも一致 |

### 2.2 制御・式・その他の言語機能

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| if ／ else if ／ for ／ while ／ break ／ continue ／ 早期 return | ◎ | 全部 | |
| switch ／ case ／ default | ◎ | 昨年作, AL3 | |
| 三項演算子 | ◎ | MT3, AL2, PG2, AL3 | |
| 範囲 for | ◎ | MT3, AL2b, AL3 | |
| イテレータ（begin/end、`*it`、`++it`、insert、erase ループ） | ◎ | PG3, AL3 | |
| auto（イテレータ、`const auto&`） | ◎ | PG3, MT3, AL3 | |
| 参照渡し（`const T&` 入力、`T&` 出力） | ◎ | 全部 | |
| ポインタ、nullptr チェック | ◎ | 全部 | |
| const 正しさ（const メンバ関数、const ローカル） | ◎ | 全部 | |
| constexpr | ✕ | MT3 のみ | **決定: 使わない**（§8）。定数は `static inline const` ／ `const` |
| enum class（列挙子は `kName`） | ◎ | AL3（多数） | |
| 非スコープ enum（添字・要素数の番兵） | ◎ | AL2, 昨年作, AL3 | |
| enum class → 整数キャストでテーブル参照 | ○ | AL3 | |
| static_cast（数値変換）／ reinterpret_cast（ImGui 用） | ◎ ／ ○ | 全部 ／ AL3 | C 形式キャストは避ける |
| 固定幅整数（uint32_t, int32_t, size_t） | ◎ | MT3, PG2, AL3 | |
| 波括弧初期化、集成体初期化、値初期化 `{}`、`return {…}` | ◎ | 全部 | |
| 指示付き初期化（`.center = …`） | ✕ | MT3（途中の章のみ） | 使わない（MT3 でも後に通常引数へ戻している） |
| 再帰 | ○ | PG3 | |
| 複合代入、インクリメント、整数除算・剰余 | ◎ | 全部 | |
| ビット演算（色の分解・合成） | ○ | AL2 | |
| 関数ポインタ、例外（try ／ catch）、クラステンプレート | ✕ | — | |

### 2.3 標準ライブラリ

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| std::vector（push_back, size, [], resize, clear, erase, 2 次元） | ◎ | PG3, MT3, AL2b, AL3 | |
| std::array | ◎ | MT3, AL3 | |
| std::list（insert、イテレータ） | ○ | PG3 | AL3 では vector ＋ erase で同じことをしている |
| std::map（[], find, at, contains） | ○ | AL3 | 文字列→enum の表など |
| std::string（+、c_str、empty、==、compare、stoi、to_string） | ◎ | PG3, MT3, AL2b, AL3 | |
| std::optional | ○ | AL3 | 振る舞いの切替要求に使用 |
| std::variant（holds_alternative, get, get_if） | ○ | AL3 | GlobalVariables |
| `<algorithm>` std::clamp | ◎ | MT3, AL3 | |
| `<algorithm>` std::min ／ std::max | ◎ | MT3, AL2 | Windows.h の min ／ max マクロと衝突するので `(std::min)(a, b)` か比較式で書く（エンジンでも同じ注意） |
| `<algorithm>` std::sort | ○ | PG3 | |
| `<algorithm>` std::swap | ○ | MT3 | |
| `<cmath>` sqrt ／ sin ／ cos ／ tan ／ fabs ／ abs ／ atan2 ／ asin ／ floor ／ ceil ／ pow ／ std::lerp | ◎ | 全部 | |
| 円周率: `std::numbers::pi_v<float>`（`<numbers>`）／ `M_PI`（`_USE_MATH_DEFINES`） | ◎ | MT3, AL3 ／ AL2, PG2, 昨年作 | 新作は `std::numbers::pi_v<float>` に統一 |
| 乱数: `<random>`（mt19937, random_device, uniform_real_distribution） | ○ | AL3 | |
| 乱数: rand() ／ srand(time) ／ `rand() % n + min` | ◎ | AL2, PG2, 昨年作 | |
| `<fstream>` ／ `<sstream>`: ifstream → stringstream → getline（カンマ区切り）で CSV 読込 | ○ | AL3 | 調整値の読込にも使う |
| `<fstream>` ofstream で書き出し、`<iomanip>` setw | ○ | AL3 | 調整値の CSV 保存に使う |
| `<filesystem>`（exists, create_directory, directory_iterator, extension, stem） | ○ | AL3 | |
| `<format>` std::format | ○ | AL3 | エンジンの Log と相性がよい |
| nlohmann::json | ✕ | AL3 | **決定: 使わない**（§8）。自作エンジンの externals に無いため、調整値は CSV にする |
| `<cassert>` assert（`assert(cond && "説明")`） | ◎ | MT3, AL2b, PG2, AL3 | |
| memcpy、_countof、printf 書式 | ◎ | 複数 | エンジンでは Input が入力を管理するので memcpy の出番は少ない |
| std::function、std::unordered_map、std::pair ／ tuple、std::shared_ptr、std::chrono、スレッド、std::find ／ remove_if ／ for_each、std::string_view ／ span | ✕ | — | |

### 2.4 プリプロセッサ・ビルド

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| `#pragma once`、`#pragma region`（日本語見出し可） | ◎ | 全部 | |
| `#ifdef` による Debug ／ ImGui コードの切替 | ◎ | MT3, AL2b, AL3 | 自作エンジンでは ImGui は `#ifdef USE_IMGUI`、開発中だけのコードは `#ifndef NDEBUG`（`_DEBUG` は使わない） |
| `#define NOMINMAX` | ○ | AL3 | エンジン側で min ／ max マクロが生きているので、代わりに `(std::min)` か比較式 |
| XML ドキュメントコメント `/// <summary>` | ◎ | MT3, AL2b, AL3 | |
| ヘッダと実装の分離、1 クラス 1 ファイル組 | ◎ | AL2, PG2, MT3, AL3 | |

## 3. 数学・物理・当たり判定

### 3.1 ベクトル・行列・座標変換

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| ベクトル演算（加減、スカラー倍、内積、外積、長さ、距離、正規化（0 除算防止）、射影、反射、最近接点、垂直ベクトル） | ◎ | MT3, PG2, AL2, 昨年作 | エンジンにも Vector3 用がある。Vector2 用は無いので Dot ／ Length ／ Normalize は自前（MT3 の知識の 2D 版） |
| 行列（加減乗、逆行列、転置、単位、平行移動、拡縮、回転 X/Y/Z、X→Y→Z 合成、アフィン S·R·T、透視投影、正射影、ビューポート、Transform（w 除算）） | ◎ | MT3, PG2, AL3 | エンジンに同名関数あり（同じ行ベクトル規約） |
| ビュー行列（カメラ行列の逆行列 ／ 手組み）、注視点（LookAt）行列 | ◎ ／ ○ | MT3, PG2 ／ MT4 | |
| レンダリングパイプライン（ローカル→ワールド→ビュー→射影→NDC→スクリーン） | ◎ | MT3, PG2 | |
| ワールド→スクリーン（Transform 2 段 ／ VP×Viewport の事前合成） | ◎ | MT3, PG2 | 2D ではカメラ座標の減算（AL2, 昨年作） |
| NDC→ワールドの逆変換（`Inverse(VP)`） | ○ | MT3（FrustumDebug） | **決定: 線分–平面の交点（MT3_02_03）と組み合わせて、スクリーン→ワールド変換に使ってよい**（§8） |
| 階層構造（親子行列 `local × parentWorld`） | ○ | MT3 | エンジンの Object3D に親子付けは無いので自前で合成 |
| 球面座標 ⇄ 直交座標 | ○ | MT4 | |
| 視錐台 6 平面の抽出、線分クリッピング、球–視錐台 | ○ | MT3（定義のみ・未使用） | §8（未確認） |
| 2D 回転（頂点回転、N 方向ベクトル）、三角関数による円運動・公転 | ◎ | AL2, 昨年作, AL3, MT3 | |
| 度⇄ラジアン、向きの角度規約 | ◎ | AL3 | |
| 画角から画面端のワールド座標を求める（`tan(fovY/2)·距離`） | ○ | AL3 | |
| 整数の桁分解（スコア表示） | ○ | 昨年作 | |

### 3.2 補間・曲線・時間

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| 線形補間 Lerp（float ／ ベクトル） | ◎ | MT3, PG2, AL3, 昨年作 | エンジンにもあり |
| イージング: EaseInBack（AL2）、EaseInOut 区分 2 次（PG2）、smoothstep（AL3）、ease-out ／ ease-in quad（昨年作） | ◎ | 左記 | エンジンには無いのでゲーム側に Easing を置く |
| 正規化時間 t = timer ／ duration（0〜1 に clamp）で進める演出 | ◎ | PG2, AL3, 昨年作 | |
| 2 次ベジェ曲線（De Casteljau＝Lerp を 2 段） | ◎ | MT3（MT3_03_00） | エンジンにも `Bezier()` ／ `Curve` あり。**3 次ベジェ、Catmull-Rom、スプライン、弧長パラメータ化は ✕** |
| 固定フレーム時間（1/60 秒を足す ／ 1 フレームあたりの移動量） | ◎ | MT3, PG2, AL3, 昨年作 | エンジンも 60 回/秒固定 |
| サイン波の揺れ、往復 `sin(πt)` | ◎ | AL2, AL3 | |

### 3.3 運動・物理

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| オイラー積分（v += a、p += v） | ◎ | MT3, AL2, PG2, 昨年作, AL3 | |
| 重力、終端速度（最大落下速度） | ◎ | AL2, AL3, 昨年作 | |
| ジャンプ初速、接地判定、空中ジャンプ防止 | ◎ | AL2, AL3, 昨年作 | |
| 可変ジャンプ（押した長さで高さが変わる） | ◎ | 昨年作（離した瞬間に速度を補正）、AL3（離したら上昇速度を 0.8 倍） | 新作はどちらの方式でも可 |
| コヨーテタイム | ○ | AL3 | |
| 加速・減速（摩擦）・最高速度・逆入力ブレーキ | ○ | AL3 | |
| 速度・位置のクランプ（画面端、マップ端） | ◎ | 全部 | |
| 反発（Reflect ＋ 反発係数、めり込みの押し戻し） | ◎ | MT3, PG2 | |
| ノックバック（初速 ＋ 減衰 ＋ 無敵時間） | ○ | AL3 | |
| ばね、単振り子、円錐振り子、等速円運動 | ○ | MT3 | |
| 入力ベクトルの正規化（斜め移動の速度統一） | ◎ | AL2b, PG2, 昨年作 | |

### 3.4 当たり判定

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| 円–円（2D）／ 球–球（3D） | ◎ | AL2, PG2, 昨年作, MT3 | 新作の攻撃・ブーメラン判定の基本。エンジンの Sphere–Sphere を z=0 で使ってもよい |
| 矩形–矩形（AABB） | ◎ | AL2, AL3, MT3 | |
| 円–矩形（最近点法） | ○ | AL2 | |
| 距離の二乗で比較して sqrt を避ける | ○ | AL2 | |
| 3D: 球–平面、カプセル–平面、線分–平面、線分–三角形、AABB–球、AABB–線分（スラブ法）、OBB–球、OBB–線分、OBB–OBB（分離軸）、OBB–AABB | ◎ | MT3 | エンジンの `IsCollision` に同じ組み合わせあり |
| マップチップとの判定（軸分離の押し戻し、4 隅 ／ 多点サンプル、方向別の順序、坂、サブステップ） | ◎ | AL2, AL3 | |
| 判定の総当たり（同種ペアの重複排除 `j = i + 1`） | ◎ | MT3, AL3 | |
| 判定結果の可視化（ヒットで色変更） | ◎ | MT3, AL2 | |

### 3.5 カメラ

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| 追従（対象 ＋ オフセット、補間、先読み）、移動範囲のクランプ | ◎ | AL2, AL3, 昨年作 | |
| 1 軸ごとの慣性付き追従（FollowAxis） | ○ | AL3 | |
| カメラシェイク（乱数オフセット ＋ タイマー） | ○ | AL2 | |
| 強制スクロールと画面内制限 | ○ | AL3 | |
| フリーカメラ ／ デバッグカメラ切替 | ◎ | MT3, AL3 | エンジンに DebugCamera あり |

## 4. ゲームの構造・実装パターン

### 4.1 骨組み・シーン・状態

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| 初期化 ／ 更新 ／ 描画の分離（Initialize ／ Update ／ Draw） | ◎ | 全部 | 自作エンジンでは BaseScene::OnInitialize ／ OnUpdate ／ OnDraw に対応 |
| シーン管理: enum ＋ switch 方式 | ◎ | 昨年作, AL3 | |
| シーン管理: 基底クラス ＋ SceneManager（遅延切替、unique_ptr ／ 生ポインタ） | ◎ | MT3, PG2 | エンジンの SceneManager ／ BaseScene はこの形。そのまま使う |
| シーン間のデータ受け渡し（コンストラクタ引数、main が所有する共有オブジェクト） | ◎ | PG2, AL3 | エンジンでは `ChangeScene(std::make_unique<T>(引数))` |
| フェーズ管理（enum class Phase ＋ switch） | ◎ | AL3, 昨年作（フラグ） | |
| 振る舞いステートマシン（Behavior ＋ optional の切替要求 ＋ 状態別 Update 関数） | ○ | AL3 | ボスの AI にそのまま使える形 |
| フラグ ＋ フレームカウンタの状態管理 | ◎ | AL2, 昨年作 | |
| 攻撃の多段フェーズ（予備動作→本体→余韻） | ○ | AL3 | |
| ポーズ（更新の早期 return、復帰先の記憶） | ◎ | PG2, 昨年作 | |
| 開始演出（カメラの補間移動とスキップ） | ○ | PG2 | |
| ゲームオーバー ／ クリア条件とリザルトへの遷移 | ◎ | PG2, 昨年作 | |

### 4.2 入力

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| 押下中 ／ 押した瞬間 ／ 離した瞬間の使い分け | ◎ | 全部 | エンジン: Input::IsPress ／ IsTrigger ／ IsRelease |
| 反対キー同時押しの相殺 | ◎ | AL2, PG2, MT3 | |
| 長押し時間の計測（ジャンプ受付窓） | ◎ | 昨年作, AL3 | |
| マウス: 位置、左右ボタンの押下 ／ トリガー、ホイール蓄積とクランプ | ○ | 昨年作 | エンジン: Input::GetMousePosition ／ IsMousePress ／ IsMouseTrigger ／ IsMouseRelease ／ GetWheel |
| 押下→離しの検出をフラグで作る | ○ | 昨年作 | エンジンでは IsMouseRelease で直接取れる |
| マウス位置と比較して向きを決める | ○ | 昨年作 | |
| ゲームパッド | ✕ | （AL3 完成版は ex3 以降） | 今回は使わない（エンジンにはあるが、知識としては範囲外） |

### 4.3 オブジェクト管理・所有

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| 固定長配列 ＋ isAlive ／ isActive のプール、再出現タイマー | ◎ | AL2, AL2b, PG2, 昨年作 | |
| `std::vector<Base*>` に生成して所有者が delete | ◎ | PG2, AL3 | 新作では `std::vector<std::unique_ptr<Base>>` にする |
| vector の erase ループで終了したものを除去 | ○ | AL3 | |
| リングバッファ（残像） | ○ | AL2 | |
| 所有と委譲（Initialize(model, position, camera) 型、カメラやモデルはポインタで渡す） | ◎ | AL2, AL3 | |
| 敵の基底クラスと派生、結果を enum（HitResult）で返す | ○ | AL3 | ShieldEnemy＝正面からは倒せない敵 |
| エフェクトの基底クラスと統合リスト、static の共有リソース | ○ | AL3 | |
| 集約クラスで一括 Update ／ 判定 ／ Draw | ○ | MT3（Objects） | |
| static メンバのマネージャ、クラス内包のプール（弾） | ○ | PG2 | |
| データ駆動（ステージテーブル ／ スポーン情報から生成） | ◎ | AL2b, AL3 | |

### 4.4 アクション（移動・攻撃・投擲）

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| 左右移動、向きの記憶、投げ中の向きロック | ◎ | AL2, 昨年作, AL3 | |
| 近接攻撃: 持続タイマー、向きで判定位置をオフセット、地上 ／ 空中で分岐、排他制御 | ◎ | 昨年作, AL2, AL3 | |
| 空中攻撃: プレイヤー中心の判定（公転 ／ 大きい円） | ◎ | 昨年作, AL2 | |
| 急降下攻撃、残像 | ○ | AL2 | |
| 投擲物: 長押しで狙い→離して発射、往路 ease-out ／ 復路 ease-in、復路はプレイヤーを追尾 | ○ | 昨年作 | ブーメランの土台 |
| 弾: プール、発射位置、画面外で無効化 | ○ | PG2 | |
| ヒット時の処理（撃破、スコア、消滅、1 ヒットで break） | ◎ | PG2, 昨年作, AL3 | |
| ガード（向かい合い判定）、のけぞり、ノックバック | ○ | AL3 | |
| やられ演出（飛び上がって落下、回転、フェード、パーティクル） | ○ | AL3 | |

### 4.5 マップ・ステージ・データ

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| マップチップ: int 二次元配列 ＋ enum 記号（ソース直書き） | ○ | AL2 | |
| マップチップ: CSV 読込（2 文字コード）、インデックス⇄ワールド座標、セル矩形 | ○ | AL3 | |
| 同じ CSV からオブジェクト配置（P0 ／ E0 …）を読む | ○ | AL3 | |
| 地形種ごとのモデル切替・回転、ブロックの大量配置 | ○ | AL3 | |
| 複数ステージ（一覧 CSV ＋ StageManager） | ○ | AL3 | |
| ホットリロード（ImGui ボタンで CSV 再読込） | ○ | AL3 | |
| 調整項目の一元管理（GlobalVariables: 登録 ／ 反映 ／ ImGui 編集 ／ 保存 ／ 読込） | ○ | AL3 | **保存形式は CSV（決定）**。fstream だけで書ける |
| デバッグ起動設定（CSV で開始ステージ指定） | ○ | AL3 | |
| 背景の視差スクロール（レイヤー） | △ | （AL3 完成版は ex3 以降） | 複数レイヤーを奥行きの違う Z に置いて描くだけなら ◎（描画順・座標変換の知識で足りる）。§8（未確認） |

### 4.6 演出・UI・音

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| パーティクル（プール、寿命、アルファフェード、乱数速度、8 方向飛散） | ◎ | AL2, AL3 | |
| ヒットエフェクト（拡大→フェード、再利用型 ／ 使い捨て型） | ○ | AL3 | |
| 半透明の描画順（不透明→半透明→最前面） | ○ | AL3 | |
| フェード（画面サイズのスプライト ＋ アルファ） | ○ | AL3 | |
| スプライトシートのコマ送り（UV オフセット） | ○ | 昨年作 | エンジン: Sprite::GetUVTransform |
| 左右反転描画 | ○ | 昨年作（頂点順の入替） | エンジンでは UV の scale.x = -1 を試す（動作は要確認） |
| 数字スプライトによるスコア表示、HP アイコン | ○ | 昨年作 | |
| HP バー（矩形の幅＝HP）、点滅テキスト | ○ | PG2 | |
| UI のスライドイン→待機→スライドアウト | ○ | 昨年作 | |
| タイトル画面（モデル表示 ＋ 揺れ ＋ キーで開始） | ◎ | AL3, 昨年作 | |
| 音: 読込ハンドルと再生ハンドル、二重再生防止、BGM ループ ／ SE | ○ | 昨年作（Novice） | エンジンの Audio は Play ／ SetVolume のみ。**決定: 当面は SE だけ**（§8） |
| 天球 | ○ | AL3 | |
| 自作マウスカーソル | ○ | 昨年作 | エンジンにカーソル非表示の API は無い（Win32 を直接呼ぶなら △） |

### 4.7 デバッグ・調整

| 項目 | 判定 | 出典 | メモ |
| --- | --- | --- | --- |
| ImGui: Begin ／ End、Text、Button、Checkbox、Slider 系、Drag 系、InputFloat、TreeNode、PushID、Separator、MenuBar | ◎ | MT3, AL2b, AL3 | 自作エンジンでは OnDrawImGui に書き、`#ifdef USE_IMGUI` で囲む |
| デバッググリッド、ワイヤ球、線分描画、判定の可視化 | ◎ | MT3, PG2, AL2 | エンジン: DebugDraw |
| 画面へのデバッグ文字（ScreenPrintf） | ◎ | MT3, PG2 | エンジンには無い → ImGui::Text か Log で代用 |
| Debug ／ Release の切替（`#ifdef`） | ◎ | MT3, AL3 | |

## 5. 自作エンジンの機能（ゲーム側から使えるもの・無いもの）

詳細は [knowledge/Engine.md](knowledge/Engine.md) と `Engine/Docs/EngineReference.html`。ここでは今回のゲームに効く点だけ。

**ある（そのまま使う）**

- 骨組み: `Framework`（MyGame が継承）、`SceneManager` ／ `BaseScene`（OnInitialize ／ OnUpdate ／ OnDraw ／ OnDrawImGui、`ChangeScene<T>()`、描画先矩形 `GetRenderArea*`）
- 入力: キーボード（IsPress ／ IsTrigger ／ IsRelease）、**マウス（座標・左右中ボタン・ホイール）**、ゲームパッド
- 2D: `Sprite`（位置・倍率・回転・色・UV 変換・サイズ変更。回転軸は左上固定）、`SpriteEditor`（仮絵をその場で描いて PNG 保存）
- 3D: `Object3D`（組み込み形状 kCube ／ kSphere ／ kPlane と OBJ、色、UV）、`camera_`（位置・回転・画角・クリップ面。透視投影のみ）、平行光源 ／ 点光源、`DebugCamera`
- 線描画: `DebugDraw`（線分・球・AABB ／ OBB・三角形・カプセル・平面・ベジェ曲線・グリッド。全構成で使え、OnDraw の最後に深度無視で描かれる）
- 数学: Vector2 ／ Vector3 の演算子、Vector3 の Dot ／ Cross ／ Length ／ Normalize ／ Project ／ Reflect ／ Lerp、行列一式（Inverse ／ Transpose ／ 平行移動 ／ 拡縮 ／ 回転 ／ アフィン ／ 透視 ／ 正射影 ／ ビューポート ／ Transform）、`ColorFromHex`
- 形状と判定: Shapes.h の 3D ／ 2D 形状、`IsCollision` の 3D 全組み合わせ、`MakeAABB` ／ `MakeOBB` ／ `MakePlane` ／ `ClosestPoint`
- 曲線: `Bezier(p0, p1, p2, t)`、`Curve`（制御点 3 つ、ImGui 編集つき）
- 資産: `TextureManager`（Load ／ Reload ／ 動的テクスチャ ／ 実寸取得）、OBJ 読込、`Audio`（LoadWave ／ Play ／ SetVolume）
- その他: `Time`（60 回/秒固定、GetDeltaTime）、`Log`、ImGui（Debug ／ Development のみ）、視錐台カリング

**無い（ゲーム側で作る。作り方の知識は §2〜§4 にある）**

- スクリーン座標 ⇄ ワールド座標の変換（マウスで狙うのに必須。`Inverse(view × proj)` ＋ `Transform` ＋ `GetRenderArea*` で自前）
- 2D 形状同士の当たり判定関数（Circle–Circle 等。型だけある → z=0 の球 ／ AABB3D で代用するか自前）
- Vector2 の Dot ／ Length ／ Normalize ／ Lerp、Clamp、イージング、乱数、円周率定数、度⇄ラジアン、2D 回転
- 画面座標（2D）の線・矩形・円の描画。製品用の太い線（DebugDraw は 1px）
- パーティクル、インスタンス描画、ビルボード、残像、Object3D の親子付け、正射影カメラ、注視点、カメラシェイク ／ 追従のヘルパー
- シーン遷移の演出（フェード）、ポーズのためのシーンスタック、タイムスケール ／ ヒットストップ、タイマークラス
- 音の Stop ／ ループ再生（BGM）／ IsPlaying ／ 音声キャッシュ（ゲーム側からは追加できない）
- CSV ／ JSON の読込、セーブデータ、文字・数字の描画、Sprite のアンカー ／ 反転フラグ ／ レイヤー順
- カーソルの表示 ／ 非表示

## 6. KamataEngine → 自作エンジン 置き換え表（AL3 の知識を移植するとき）

| AL3（KamataEngine）での書き方 | 自作エンジンでの書き方 |
| --- | --- |
| `KamataEngine::Initialize` → 自前の while ループ → `Finalize` | `MyGame : Framework` の `Run()` がループを回す。ゲーム側はシーンだけ書く |
| `DirectXCommon::PreDraw()` ／ `PostDraw()`、`ImGuiManager::Begin/End/Draw` | エンジンが呼ぶ。ゲームは `OnDraw` ／ `OnDrawImGui` の中身だけ書く |
| `enum class Scene` ＋ switch ＋ 生ポインタ、`IsFinished()` で終了通知 | `BaseScene` を継承したシーンで `ChangeScene<次のシーン>()` を呼ぶ（予約制、次フレームで切替） |
| `Model::CreateFromOBJ("name", true)` ／ `Model::Create()` ＋ `WorldTransform` ＋ `model->Draw(wt, camera[, &color])` | `Object3D obj; obj.Initialize("resources/xxx.obj")` または `Initialize(Primitive::kCube)`、位置は `GetTransform().translate`、色は `SetColor`、毎フレーム `Update()` → `Draw()` |
| `worldTransform_.matWorld_ = MakeAffineMatrix(…); TransferMatrix();` | `Object3D::Update()` が S·R·T を作る。自前の行列は渡せない（親子付けは自前で座標を合成） |
| `Model::PreDraw(CullingMode::kNone, BlendMode::kNormal, DepthTestMode::kOff)` | 無し。半透明は描画順で対応、深度無視の前面描画は DebugDraw だけ |
| `Camera camera_; camera_.translation_; camera_.fovAngleY; camera_.UpdateMatrix()` | `BaseScene::camera_`（`GetTransform().translate` ／ `rotate`、`SetFovY`）。行列はエンジンが更新 |
| `DebugCamera` の `matView` をコピーして `TransferMatrix()` | `CalcViewMatrix()` をオーバーライドして `DebugCamera::GetViewMatrix()` を返す |
| `ObjectColor` ＋ `SetColor(Vector4)` | `Object3D::SetColor(Vector4)`（アルファで半透明） |
| `TextureManager::Load("white1x1.png")` → `Sprite::Create(handle, pos)` → `SetSize` ／ `SetColor` → `Sprite::PreDraw` ／ `Draw` ／ `PostDraw` | `Sprite s; s.Initialize("resources/xxx.png", size)`、`GetTransform().translate`、`SetSize` ／ `SetColor`、毎フレーム `Update()`、`OnDraw` の最後に `Draw()`（PreDraw ／ PostDraw は不要） |
| `Input::GetInstance()->PushKey ／ TriggerKey(DIK_*)` | `Input::GetInstance()->IsPress ／ IsTrigger ／ IsRelease(DIK_*)` |
| （Novice）`IsTriggerMouse(0)` ／ `IsPressMouse(1)` ／ `GetWheel()` ／ `GetMousePosition(&x, &y)` | `IsMouseTrigger(kMouseLeft)` ／ `IsMousePress(kMouseRight)` ／ `IsMouseRelease(kMouseRight)` ／ `GetWheel()`（1 目盛り ±120）／ `GetMousePosition()`（描画先の左上 `GetRenderAreaX/Y` を引いてから使う） |
| （Novice）`Novice::DrawLine` で 3D ワイヤ描画 | `DebugDraw::DrawLine ／ DrawSphere ／ DrawCurve` |
| （Novice）`PlayAudio(handle, loop, vol)` ／ `StopAudio` ／ `IsPlayingAudio` | `Audio::GetInstance()->LoadWave`（アプリで 1 回）／ `Play` ／ `SetVolume`。Stop ／ ループ ／ 再生中判定は無い |
| 自作 `Matrix4x4.h` の MakeAffineMatrix 等（KamataEngine::Matrix4x4 用） | `Engine/Math/Matrix4x4.h` に同名関数あり（同じ行ベクトル規約）。ベクトルは Engine::Vector3 の演算子 |
| `WinApp::kWindowWidth ／ kWindowHeight` | `WinApp::kClientWidth ／ kClientHeight`（1280×720。スプライトの基準解像度） |
| `std::ifstream` ＋ `std::stringstream` で CSV | そのまま使える（実行時カレントは `Game/`、パスは `"resources/…"`） |
| `nlohmann::json` | 使わない（調整値は CSV。§8） |
| `#ifdef _DEBUG` ／ `#ifdef USE_IMGUI` | `#ifndef NDEBUG`（開発中だけのコード）／ `#ifdef USE_IMGUI`（ImGui） |

## 7. 今回の構想との突き合わせ

| 構想の要素 | 必要な知識 | 判定 |
| --- | --- | --- |
| 3D 描画の 2D 横視点（Z=0 平面で遊ぶ） | PG2（Z=0 平面のゲームを 3D 投影で表示）、AL3 | ◎ Object3D ＋ `camera_`（回転 0 で +Z を向け、横から見る） |
| A ／ D 移動、加減速 | AL3、昨年作 | ◎ |
| W ／ Space の可変ジャンプ | 昨年作、AL3 | ◎ |
| 空中で長押しするとゆっくり降下 | 重力の係数 ／ 終端速度を押下中だけ変える（AL2 ／ AL3 の重力・終端速度の応用） | ◎ |
| 近接攻撃（地上: 正面の円 0.5 秒、空中: 大きい円 0.8 秒） | 昨年作（keepTimer ／ 向きオフセット ／ 地上空中分岐）、円–円判定 | ◎ |
| 右長押しで狙う（カーソル位置が目標） | マウス入力（昨年作、エンジン Input）、スクリーン→ワールド変換（§8 で承認済み） | ◎ |
| 予測線の表示 | DebugDraw::DrawLine ／ DrawCurve（MT3 の線描画の知識）。製品用に太くするなら小さな球 ／ スプライトを曲線上に並べる | ◎ |
| ホイールで予測線をベジェ曲線化（上限あり） | GetWheel の蓄積とクランプ（昨年作）、2 次ベジェ（MT3） | ◎ 制御点を中点から上下にずらす |
| ブーメラン（曲線に沿って飛び、プレイヤーへ戻る、円判定） | 昨年作の 2 段階補間（往路 ease-out ／ 復路 追尾 ease-in）＋ ベジェ | ◎ |
| ボス（HP、攻撃パターン、弱点） | enum class ＋ switch の状態機械（AL3 Behavior）、BaseEnemy 派生、HitResult | ◎ |
| タイトル → ボス → クリア | エンジンの SceneManager（MT3 ／ PG2 で同型を学習）、フェード（AL3） | ◎ |
| マップチップのレイヤー（手前・奥） | CSV 読込（AL3）＋ Z 位置を変えて描く | ◎（視差を付けるなら §8 未確認） |
| 調整項目（ImGui → ファイル保存） | AL3 GlobalVariables の仕組み ＋ CSV（fstream） | ◎ CSV に決定 |
| 効果音 ／ BGM | Audio は Play ／ SetVolume のみ | ○ 当面 SE のみ（決定） |
| スコア ／ HP などの数字 UI | 数字スプライト ＋ 桁分解（昨年作）、Sprite の UV 切り出し | ◎ |

## 8. 要確認事項と決定

### 決定済み（2026-10-07）

1. **std::unique_ptr ／ make_unique** — 使う。所有は unique_ptr、借りるだけの相手は生ポインタ（MT3 の SceneManager で習得済み。エンジンの方針とも一致）
2. **ラムダ式** — 使わない。共通化は名前付きの関数で
3. **constexpr** — 使わない。定数は `static inline const` ／ `const`
4. **nlohmann::json** — 使わない。調整値の保存・読込は CSV（fstream）
5. **スクリーン → ワールド変換** — `Inverse(VP)` で NDC→ワールド（MT3 FrustumDebug）＋ 線分–平面の交点（MT3_02_03）の組み合わせで実装してよい
6. **BGM** — 当面は SE のみ。エンジンの Audio にループ ／ 停止が無いため、BGM が必要になったら別作業（feature ブランチ）として判断
7. **指示付き初期化**・**ゲームパッド** — 使わない（当面の扱い。必要になったら改めて確認）

### 未確認（当面の扱い）

- **MT3 で書いたが使っていない関数**（視錐台の抽出、線分クリッピング、ToSpherical）— 本人が書いたコードなので使える前提。使う場面が来たら一言確認
- **背景の視差スクロール** — 単純な多層描画は既知の知識で可能。カメラ移動量に係数を掛ける視差はレイヤー着手時に判断
- **AL3 の「調整」コミット由来の技術**（坂地形、コヨーテタイム、サブステップ、FollowAxis の慣性）— 本人が書いたコードなので使える前提。授業外の技法だと分かったら差し替える
- **AL2_02 の正体** — ウィンドウタイトルが `AL3_5_1_04_suzuki`。AL3 の課題の可能性があるが、いずれにせよ本人の過去作として扱う（確認のみ）

## 9. 未習得（現時点では使わない）

- 言語: クラステンプレート、関数ポインタ、例外、friend、explicit ／ noexcept ／ static_assert、dynamic_cast、構造化束縛、（決定により）ラムダ式・constexpr・指示付き初期化
- 標準ライブラリ: std::function、std::unordered_map、std::pair ／ tuple、std::shared_ptr、std::chrono、スレッド ／ 非同期、std::find ／ remove_if ／ for_each、std::string_view ／ span、`<regex>`、（決定により）nlohmann::json
- 数学: 3 次ベジェ、Catmull-Rom ／ スプライン、弧長パラメータ化（等速移動）、クォータニオン、Slerp、任意軸回転
- 描画 ／ エンジン: スキニング ／ アニメーション、ビルボード、インスタンス描画、シェーダ ／ PSO の自前追加（制作中はエンジンを触らないため対象外）、ゲームパッド入力
- 設計: State パターン（クラスで状態を表す）→ enum class ＋ switch ＋ 状態別関数で書く

## 10. 更新履歴

- 2026-10-07: 初版。PG3 ／ MT3・MT4 ／ AL2_01・AL2_02・PG2 ／ 昨年作 ／ AL3（〜ex3）／ 自作エンジン を棚卸し
- 2026-10-07: 要確認事項の判断を反映（unique_ptr 使用、ラムダ・constexpr・指示付き初期化・json・パッド不使用、スクリーン→ワールド変換の承認、SE のみ）
