# AL3（KamataEngine）知識項目インベントリ — タグ `AL3_05_19_ex3` 時点

## 0. 対象と読み方

- **対象**: `C:\Users\s-dai\source\repos\AL3` の worktree `C:\Users\s-dai\source\repos\AL3_ex3`（detached HEAD `ea88269` = tag `AL3_05_19_ex3`）。以下の行番号はこの worktree のファイルを指す。
- **読んだ学生コード（全 40 ファイル・5,206 行）**: main.cpp / GameScene.h,.cpp / TitleScene.h,.cpp / Player.h,.cpp / BaseEnemy.h,.cpp / Enemy.h,.cpp / ShieldEnemy.h,.cpp / MapChipField.h,.cpp / CameraController.h,.cpp / Skydome.h,.cpp / Fade.h,.cpp / Easing.h,.cpp / AttackEffect.h,.cpp / BaseEffect.h / GuardEffect.h,.cpp / HitEffect.h,.cpp / DeathParticles.h,.cpp / StageManager.h,.cpp / GlobalVariables.h,.cpp / Matrix4x4.h,.cpp / Vector3.h / AABB.h。
- **読んでいないもの**: KamataEngine 本体（`$(ProjectDir)..\External\KamataEngine` — このワークツリーには存在しない。DirectXGame.vcxproj:62）、Resources/shaders（エンジン付属の HLSL）。
- **ビルド設定の事実**: C++20（`<LanguageStandard>stdcpp20` DirectXGame.vcxproj:89）、Debug 構成で `USE_IMGUI` 定義（DirectXGame.vcxproj:85）、リンク `KamataEngine.lib; DirectXTex.lib`（DirectXGame.vcxproj:96）。
- 各項目の形式: `- **項目名** — 内容 — 根拠: ファイル:行`。

---

## A. C++ 言語機能・標準ライブラリ

### A-1. クラス設計・言語機能

- **class / struct** — ゲームオブジェクトは class、データの束は struct（AABB, StageData, TerrainShape, IndexSet, Rect, MapObjectSpawn, CollisionMapInfo） — 根拠: Player.h:11, AABB.h:7, StageManager.h:8, MapChipField.h:30/52/60/66, Player.h:152
- **ネストした型（クラス内 enum / struct）** — `Fade::Status`, `Player::CollisionMapInfo`, `CameraController::Rect`, `BaseEnemy::HitResult` をクラス内で定義 — 根拠: Fade.h:10, Player.h:152, CameraController.h:14, BaseEnemy.h:60
- **public / protected / private** — 派生クラスに見せるメンバは protected（BaseEnemy の状態・定数、ShieldEnemy の UpdateWalkAnimation） — 根拠: BaseEnemy.h:82, ShieldEnemy.h:27
- **public 継承** — `Enemy : public BaseEnemy`, `ShieldEnemy : public BaseEnemy`, `HitEffect/GuardEffect : public BaseEffect` — 根拠: Enemy.h:9, ShieldEnemy.h:11, HitEffect.h:15, GuardEffect.h:12
- **仮想関数と仮想デストラクタ** — `virtual ~BaseEnemy() = default;`、Initialize/Respawn/Update/Draw/OnHitByAttack/UpdateWalkAnimation を virtual に — 根拠: BaseEnemy.h:16-102, BaseEffect.h:18
- **純粋仮想関数（抽象クラス）** — `virtual void Initialize(const Vector3&) = 0;` 等 3 つ — 根拠: BaseEffect.h:24-34
- **override 指定子** — 派生側の全オーバーライドに付与 — 根拠: ShieldEnemy.h:14-29, HitEffect.h:30-40, GuardEffect.h:24-34
- **基底クラスの実装を明示呼び出し** — `BaseEnemy::Respawn(position)`, `BaseEnemy::Update()`, `BaseEnemy::OnHitByAttack(...)` — 根拠: ShieldEnemy.cpp:31, 41, 70
- **ポリモーフィズム（基底ポインタのコンテナ）** — `std::vector<BaseEnemy*>` / `std::vector<BaseEffect*>` に派生を new して格納、基底ポインタで delete — 根拠: GameScene.h:93/100, GameScene.cpp:197-200, 551-553, 252-261
- **コンストラクタ / デストラクタ** — 空のコンストラクタと、所有ポインタを delete するデストラクタ — 根拠: GameScene.cpp:40, 241-287; TitleScene.cpp:7-13
- **`= default` / `= delete`（コピー禁止シングルトン）** — private ctor/dtor = default、コピー ctor と operator= を delete — 根拠: GlobalVariables.h:75-78
- **シングルトン（関数内 static）** — `static GlobalVariables instance; return &instance;` — 根拠: GlobalVariables.cpp:27-30
- **static メンバ関数（クラス共通処理）** — `StaticInitialize`, `RegisterGlobalVariables`, `ApplyGlobalVariables` — 根拠: HitEffect.h:23/43-45, Player.h:277-279
- **static メンバ変数のクラス外定義** — `Model* HitEffect::sModel_ = nullptr;` — 根拠: HitEffect.cpp:50-51, GuardEffect.cpp:31-32
- **`static inline` データメンバ（C++17）** — 調整項目は `static inline float kX = …;`（非 const、Apply で書き換える）、純定数は `static inline const` — 根拠: Player.h:22-140, AttackEffect.h:46-50, MapChipField.h:107-112
- **クラス内初期化子（NSDMI）** — `Model* model_ = nullptr;`, `bool hasFloor = false;` — 根拠: Player.h:16, MapChipField.h:31
- **enum class（スコープ付き列挙）** — Scene / Status / Phase / Behavior / AttackPhase / DeathCause / LRDirection / Direction / HitResult / MapChipType / MapObjectType / Mode — 根拠: main.cpp:14, Fade.h:10, Player.h:28-68, BaseEnemy.h:60-93, MapChipField.h:7/45, CameraController.h:22
- **無スコープ enum（配列添字 + 要素数番兵）** — `enum Corner { …, kNumCorner }` を配列サイズと添字に使用 — 根拠: Player.h:143-149, Player.cpp:589-595
- **enum class → 整数キャストでテーブル参照** — `table[static_cast<uint32_t>(lrDirection_)]` — 根拠: Player.cpp:337-339
- **switch 文（複数 case のまとめ・default）** — シーン / フェーズ / 地形種で分岐、case を列挙してモデルをグループ化 — 根拠: main.cpp:31-54, Player.cpp:129-143, GameScene.cpp:469-492, MapChipField.cpp:148-182
- **三項演算子** — `(dir == kRight) ? 1.0f : -1.0f` — 根拠: Player.cpp:375, BaseEnemy.cpp:63
- **参照渡し（const 参照入力 / 非 const 参照出力）** — `const AABB&`、出力引数 `float& outSurfaceY`、複数値更新 `FollowAxis(float&, float&, …)` — 根拠: AABB.h:15, MapChipField.h:93, CameraController.h:74
- **const メンバ関数** — getter / 判定関数に const — 根拠: Fade.h:46, Player.h:224-269, StageManager.h:26
- **const ローカル変数の多用** — `const float t = …;`, `const char* groupName = "Player";` — 根拠: Player.cpp:418-420, Player.cpp:15
- **new / delete（手動メモリ管理）** — シーン・モデル・敵・エフェクト・ブロックの WorldTransform を new し、所有者が delete（unique_ptr 不使用） — 根拠: main.cpp:38/45, GameScene.cpp:155, 139-144, 241-287
- **前方宣言** — `class StageManager;`, `class Player; class MapChipField;` で依存を軽くする — 根拠: GameScene.h:18, CameraController.h:5-6
- **無名名前空間（ファイルローカル）** — シーンのグローバル状態、ヘルパ関数、変換テーブル、定数 — 根拠: main.cpp:20-122, GameScene.cpp:9-38, Player.cpp:88-91, HitEffect.cpp:39-47, MapChipField.cpp:11-38, GlobalVariables.cpp:22-25
- **既存名前空間への関数追加 / using namespace** — `namespace KamataEngine { Matrix4x4 Multiply(…); }`、.cpp 先頭で `using namespace KamataEngine;` — 根拠: Matrix4x4.h:4-21, Player.cpp:11
- **using 型エイリアス** — `using Item = std::variant<…>; using Group = std::map<…>; using json = nlohmann::json;` — 根拠: GlobalVariables.h:10-12, GlobalVariables.cpp:20
- **inline 自由関数（ヘッダ定義）** — `inline bool IsCollision(const AABB&, const AABB&)` — 根拠: AABB.h:15-19
- **関数オーバーロード** — SetValue / AddItem を int32_t / float / Vector3 で多重定義 — 根拠: GlobalVariables.h:32-43
- **関数内 static 変数** — 乱数エンジンと分布を関数内 static に — 根拠: HitEffect.cpp:43-44
- **波括弧初期化 / 集成体初期化** — `Vector3 v = {0,0,0}`、`return {true, 1.0f, …};`、`MapObjectSpawn spawn{};`、`Matrix4x4 result{};`、map の初期化子リスト — 根拠: Player.h:20, MapChipField.cpp:150, MapChipField.cpp:80, Matrix4x4.cpp:9, MapChipField.cpp:15-29
- **範囲 for** — コンテナ、`std::array`、初期化子リスト `{topY, centerY, bottomY}`、`directory_iterator` を走査 — 根拠: GameScene.cpp:374, DeathParticles.cpp:40, Player.cpp:775, BaseEnemy.cpp:171, GlobalVariables.cpp:200
- **auto** — `auto& row`, `auto* obj`, `auto it = effects_.begin()` の 3 箇所のみ — 根拠: GameScene.cpp:280-281, 383
- **イテレータ削除ループ** — `it = effects_.erase(it)` else `++it` — 根拠: GameScene.cpp:383-390
- **明示的イテレータ型** — `std::map<std::string, Group>::iterator`, `json::iterator` — 根拠: GlobalVariables.cpp:129, 240
- **C 配列（テーブル）** — 角度テーブル、角オフセット、サンプル点配列 — 根拠: Player.cpp:337, 589, 611, 701
- **固定幅整数型** — `uint32_t`（インデックス・個数）、`int32_t`（ステージ番号）、`size_t`（ループ）、`UINT32_MAX`（範囲外の印） — 根拠: MapChipField.h:4, StageManager.h:10, StageManager.cpp:7, MapChipField.cpp:131
- **static_cast / reinterpret_cast** — 数値型変換に static_cast、`Vector3*` → `float*`（ImGui 用）に reinterpret_cast — 根拠: Fade.cpp:11, MapChipField.cpp:127-132, GlobalVariables.cpp:312
- **関数ポインタ・ラムダ・テンプレート・演算子オーバーロード** — 学生コードには **存在しない**（operator= の delete のみ） — 根拠: 全ファイル grep（`template`/`[](`/`operator` 該当なし、GlobalVariables.h:78 のみ）
- **プリプロセッサ** — `#pragma once`（全ヘッダ）、`#ifdef _DEBUG`、`#ifdef USE_IMGUI`、`#define NOMINMAX`、`#pragma region/endregion`（日本語名も）、`#include <Windows.h>` — 根拠: main.cpp:85/157/182, Player.cpp:1, GameScene.cpp:42-109, Player.cpp:148, GlobalVariables.cpp:3-4
- **Windows API** — `WinMain(_In_ HINSTANCE, …)`、`MessageBoxA`、ワイド文字列 `L"…"` — 根拠: main.cpp:125, GlobalVariables.cpp:180/228/322, main.cpp:128
- **assert（`<cassert>`）** — NULL チェック、`assert(cond && "メッセージ")` 慣用句、`assert(false && "…")`、`assert(0)` — 根拠: Player.cpp:95, StageManager.cpp:27, StageManager.cpp:15, GlobalVariables.cpp:181
- **XML ドキュメントコメント** — `/// <summary>` `<param>` `<returns>` — 根拠: Player.h:198-204, StageManager.h:21-29

### A-2. 標準ライブラリ

- **std::vector** — 1 次元（敵・エフェクト・ステージ）と 2 次元（`vector<vector<MapChipType>>`, `vector<vector<WorldTransform*>>`）、resize / clear / push_back / size / erase — 根拠: GameScene.h:93-102, MapChipField.h:40, GameScene.cpp:118-123, StageManager.cpp:69
- **std::array** — 固定数パーティクルの WorldTransform / スケール — 根拠: DeathParticles.h:54, HitEffect.h:76-78（Player.cpp:7 は include のみで未使用）
- **std::map** — 文字列→enum の変換テーブル、グループ→項目の 2 段 map、`operator[]` によるデフォルト生成、find/at/contains — 根拠: MapChipField.cpp:15-36, GlobalVariables.h:81, GlobalVariables.cpp:34/68/95/93
- **std::string** — 連結 `+`、`c_str()`、`empty()`、`compare()`、`==`、`std::stoi` — 根拠: GameScene.cpp:132, GameScene.cpp:332, main.cpp:102, GlobalVariables.cpp:208, StageManager.cpp:9/66
- **std::variant** — `variant<int32_t, float, Vector3>`、`holds_alternative`、`get<T>`、`get_if<T>` — 根拠: GlobalVariables.h:10, GlobalVariables.cpp:147/100/301-311
- **std::optional** — 振る舞いの切替要求 `optional<Behavior>`、`if (opt)` / `.value()` / `std::nullopt` — 根拠: Player.h:42, Player.cpp:111-125
- **`<algorithm>`** — `std::clamp`（速度・t・カメラ範囲）、`std::max`（落下速度・ステップ数）。`std::min` は未使用 — 根拠: Player.cpp:206/258/551-552, CameraController.cpp:81-86/120-124
- **`<cmath>`** — `std::sin/cos/tan/floor/ceil/fabs/lerp` — 根拠: Matrix4x4.cpp:50-51, Player.cpp:484/552/551/352, MapChipField.cpp:127
- **`<numbers>`（C++20）** — `std::numbers::pi_v<float>` を向き・周期・度→ラジアンに使用 — 根拠: TitleScene.cpp:28/47, Player.cpp:104/337/453, BaseEnemy.cpp:41/78/94
- **`<random>`** — `std::mt19937` + `std::random_device` + `std::uniform_real_distribution<float>` — 根拠: HitEffect.cpp:42-46
- **`<fstream>` / `<sstream>`（CSV 読込）** — `ifstream` → `stringstream << file.rdbuf()` → `getline(行)` → `istringstream` → `getline(…, ',')` — 根拠: StageManager.cpp:26-63, MapChipField.cpp:55-72, main.cpp:93-113
- **`<fstream>`（書き出し）/ `<iomanip>`** — `std::ofstream` open/fail/close、`ofs << std::setw(4) << json << std::endl` — 根拠: GlobalVariables.cpp:173-188
- **`<filesystem>`** — `path`, `exists`, `create_directory`, `directory_iterator`, `directory_entry::path`, `extension()`, `stem()` — 根拠: GlobalVariables.cpp:165-213
- **nlohmann::json** — `json::object()`, `json::array({x,y,z})`, `root[g][k] = v`, `ifs >> root`, `find`, `iterator.key()`, `is_number_integer/is_number_float/is_array/size`, `get<T>()`, `at(i)` の暗黙変換 — 根拠: GlobalVariables.cpp:133-160, 233-266
- **`<format>`（C++20）** — `std::format("{}.json saved.", groupName)` — 根拠: GlobalVariables.cpp:321
- **`<cstdint>`** — uint32_t / int32_t — 根拠: MapChipField.h:4, StageManager.h:3
- **未使用の標準機能（境界の確認用）** — std::list、std::unique_ptr/make_unique/shared_ptr、std::function、std::pair、constexpr、例外（try/catch）、std::min、std::chrono はいずれも学生コードに **出現しない** — 根拠: 全ファイル grep

---

## B. 数学・物理・当たり判定

### B-1. ベクトル・行列・補間

- **Vector3 の成分演算を手書き** — 加算ヘルパ `Add(a,b)`、中点 `(a+b)*0.5`、エンジンの演算子は使っていない — 根拠: Player.cpp:90, GameScene.cpp:549
- **4x4 行列の自作関数（行優先・行ベクトル規約）** — Multiply、MakeIdentity4x4、MakeTranslateMatrix（m[3][0..2] に平行移動）、MakeScaleMatrix、MakeRotateX/Y/ZMatrix — 根拠: Matrix4x4.cpp:8-87
- **アフィン行列 = S × (Rx × Ry × Rz) × T** — `MakeAffineMatrix(scale, rotate, translate)` — 根拠: Matrix4x4.cpp:90-105
- **毎フレーム matWorld_ を自作行列で更新して転送** — `matWorld_ = MakeAffineMatrix(…); TransferMatrix();` — 根拠: Player.cpp:150-152, GameScene.cpp:422-425, BaseEnemy.cpp:88-89
- **度→ラジアン** — `deg * (pi / 180)` — 根拠: BaseEnemy.cpp:94, ShieldEnemy.cpp:46/82
- **向きの角度規約** — 右向き π/2、左向き 3π/2、正面（カメラ向き）π — 根拠: Player.cpp:337, BaseEnemy.cpp:41, TitleScene.cpp:28
- **線形補間 + イージング** — `EaseInOut(t) = t*t*(3-2t)`（smoothstep）→ `std::lerp(start, end, eased)` — 根拠: Easing.cpp:2-5, Player.cpp:350-352
- **正規化時間 t = timer / duration を 0..1 に clamp** — 旋回・攻撃・エフェクト・やられで共通 — 根拠: Player.cpp:346-348/418, BaseEnemy.cpp:230, GuardEffect.cpp:71
- **固定フレーム時間（1/60 秒）** — 全タイマーは `+= 1.0f / 60.0f`（デルタタイム不使用） — 根拠: Fade.cpp:31, Player.cpp:344/392, BaseEnemy.cpp:81/229, TitleScene.cpp:46
- **サインカーブの揺れ** — `sin(timer * 2π / period) * amplitude`（タイトル文字の上下、敵の歩行揺れ）、`sin(π t)` で 0→最大→0 の片道（のけぞり） — 根拠: TitleScene.cpp:47, BaseEnemy.cpp:82, ShieldEnemy.cpp:82
- **2D 回転で N 方向ベクトル** — angle = 2π·i/N、`(x cos − y sin, x sin + y cos)` — 根拠: DeathParticles.cpp:66-75
- **乱数角度 [0, π)** — トゲの Z 回転 — 根拠: HitEffect.cpp:42-46, 81-82
- **画角から見える範囲を算出** — `halfH = tan(fovY/2)·distance; halfW = halfH·aspect` — 根拠: Player.cpp:481-486, CameraController.cpp:170-172

### B-2. 移動・物理

- **加速・減速（摩擦）・最高速度** — 入力で `v += a`、非入力で `v *= (1 − 減衰率)`、逆入力で急ブレーキ、`std::clamp(v, −max, max)` — 根拠: Player.cpp:165-210
- **重力と最大落下速度** — `v.y −= g; v.y = max(v.y, −limit)` — 根拠: Player.cpp:256-258, BaseEnemy.cpp:66-67
- **ジャンプ初速と可変ジャンプ** — Trigger で `v.y += 初速`、キーを離すと上昇中の v.y を 0.8 倍 — 根拠: Player.cpp:217-225, 250-253
- **コヨーテタイム** — 離地後 kCoyoteTime フレームはジャンプ可、使用で 0 に — 根拠: Player.h:78/135, Player.cpp:217-224, 852-862
- **着地時・壁接触時の横速度減衰、天井で上昇停止** — 根拠: Player.cpp:855-858, 268-275
- **ノックバック（初速 + 毎フレーム減衰 + 時間で復帰 + 無敵）** — 根拠: Player.cpp:300-333, 888-904
- **やられ演出の物理（上向き初速→重力落下、Z 回転、地形すり抜け）** — プレイヤーと敵の両方 — 根拠: Player.cpp:287-298, BaseEnemy.cpp:222-253
- **等速歩行と壁で反転する敵 AI** — 根拠: BaseEnemy.cpp:61-78, 97-149
- **強制スクロール（等速右進行・右端停止）と画面端押し込み** — 根拠: CameraController.cpp:89-105, Player.cpp:488-519

### B-3. 当たり判定

- **AABB 構造体と交差判定（各軸の区間重なり）** — 根拠: AABB.h:7-19
- **中心 ± 半幅から AABB 生成** — Player / BaseEnemy の GetAABB — 根拠: Player.cpp:869-875, BaseEnemy.cpp:193-199
- **プレイヤー × 全敵の総当たり（死んだ敵はスキップ）** — 根拠: GameScene.cpp:524-566
- **マップチップ座標系** — セル中心 = (w·x, h·(V−1−y), 0)（CSV 上段が上）、ブロック 2×2、20 行 × 100 列 — 根拠: MapChipField.cpp:104-110, MapChipField.h:107-112
- **座標→インデックス逆変換** — 半セルずらして floor、負は UINT32_MAX にして範囲外=空白扱い — 根拠: MapChipField.cpp:120-134, 93-102
- **セル矩形 Rect（left/right/bottom/top）** — 根拠: MapChipField.cpp:136-145
- **地形形状テーブル TerrainShape（床/天井面の左右端高さ 0..1、坂を含む 14 種）** — 根拠: MapChipField.h:30-37, MapChipField.cpp:147-183
- **坂の面高さを線形補間で求める** — `h = left + (right−left)·t` → `rect.bottom + h·blockHeight` — 根拠: MapChipField.cpp:185-209
- **天井坂の詰まり判定（横からのすり抜け防止）** — 根拠: MapChipField.cpp:211-224
- **4 隅（Corner enum + CornerPosition）** — 定義はあるが、現在の判定は「左右端＋中央＋狭い足場」の多点サンプル方式に発展済み — 根拠: Player.h:143-149, Player.cpp:587-596
- **方向別の判定順序（右 → 左 → 縦）** — 横を先に解決してから縦（壁を床と誤認しないため） — 根拠: Player.cpp:538-545
- **壁判定（右/左）** — 頭・中央・足元+kGroundSnap の 3 点、セル列をまたいだ時だけ壁とみなし押し戻し `wallLeft − w/2 − kBlank` — 根拠: Player.cpp:748-838
- **床判定** — 足元 4 点（全幅 2 + 狭い足場 kSupportWidth 2）で最も高い床面を採用、空中はセル境界またぎ条件で着地、接地中は ±kGroundSnap で坂に吸着 — 根拠: Player.cpp:598-656
- **天井判定** — 2 点で最も低い天井面、セル境界またぎ条件 — 根拠: Player.cpp:657-685, 730-746
- **床面の探索範囲（接地中: 上 kGroundSnap / 下 kGroundSearch、空中: 足元と少し下）** — 根拠: Player.cpp:687-728
- **サブステップ分割（トンネリング防止）** — 1 ステップ ≤ kMaxMovePerStep、塞がれた軸は以降 0 — 根拠: Player.cpp:547-585
- **めり込み防止のすき間 kBlank** — 根拠: Player.h:123, Player.cpp:653/680/790
- **接地状態は縦判定の landing から決める** — 根拠: Player.cpp:840-863
- **敵の簡易地形判定（壁 3 点で反転・床 3×2 点で着地、穴は落下）** — 根拠: BaseEnemy.cpp:97-191

### B-4. カメラ

- **追従カメラの目標座標 = 対象 + オフセット + 速度×先読み係数** — 根拠: CameraController.cpp:68-69
- **1 軸ごとの補間・慣性・加速度制限（FollowAxis）** — 進行方向のみ追従、停止時は減衰で滑る、1 フレームの速度変化を clamp — 根拠: CameraController.cpp:107-137
- **対象周囲マージンでの clamp（画面外防止）** — 根拠: CameraController.cpp:81-82, CameraController.h:108
- **移動可能範囲（マップ端から画面半分内側、マップが画面より小さい軸は中央固定）** — 根拠: CameraController.cpp:156-187
- **Reset で即座に合わせる（補間なし）** — 根拠: CameraController.cpp:139-154

---

## C. ゲームの構造・実装パターン

- **エントリーポイントとメインループ** — WinMain → `KamataEngine::Initialize` → 事前ロード → シーン生成 → `while(true){ Update で終了判定; ChangeScene; ImGui Begin; UpdateScene; ImGui End; PreDraw; DrawScene; ImGui Draw; PostDraw }` → delete → Finalize — 根拠: main.cpp:125-224
- **シーン遷移 = enum class Scene + switch + 生ポインタ（シーン基底クラスは無い）** — ChangeScene / UpdateScene / DrawScene の 3 関数、各シーンは `IsFinished()` で終了通知、終了側を delete して次を new + Initialize — 根拠: main.cpp:14-83, TitleScene.h:31, GameScene.h:148
- **シーン間で共有するものは main が所有（StageManager）** — シーンにはポインタで渡す — 根拠: main.cpp:26-27/132-133, GameScene.cpp:46
- **デバッグ起動設定（_DEBUG のみ）** — ゲームシーンから開始、gitignore 済み `debugSettings.csv` の `キー,値` で開始ステージを指定 — 根拠: main.cpp:85-121/157-170, .gitignore:365
- **シーン内フェーズ管理（enum class Phase + switch）** — Title: kFadeIn/kMain/kFadeOut、Game: kPlay/kDeath — 根拠: TitleScene.cpp:62-83, GameScene.cpp:341-369
- **振る舞いステートマシン + 次フレーム切替要求** — `behavior_` と `std::optional behaviorRequest_`、切替時に `BehaviorAttackInitialize`、Update を `UpdateRoot/UpdateAttack/UpdateKnockback/UpdateDead` に分割 — 根拠: Player.cpp:109-143, Player.h:175-187
- **攻撃の 3 フェーズ（予備動作→突進→余韻）とフェーズ間を滑らかにつなぐスケール補間** — 根拠: Player.cpp:386-470
- **オブジェクトの標準インターフェース Initialize(model, position, camera) / Update() / Draw()** — モデルは GameScene が所有してポインタで渡す — 根拠: Player.cpp:93-107, BaseEnemy.cpp:10-25, DeathParticles.cpp:31-50
- **所有権: 生成者が delete** — GameScene がモデル・敵・エフェクト・ブロック・フェードを delete — 根拠: GameScene.cpp:241-287
- **敵の基底クラスと派生** — BaseEnemy（歩行・地形・やられ）/ Enemy（そのまま）/ ShieldEnemy（ガード・揺れ軸・Respawn をオーバーライド）、結果を `HitResult` で返して GameScene が分岐 — 根拠: BaseEnemy.h, Enemy.h:9-15, ShieldEnemy.h:11-44, GameScene.cpp:539-559
- **エフェクトの基底クラスと統合リスト** — BaseEffect（純粋仮想 + 共通の色/終了フラグ）、生成→Update→IsFinished で delete + erase、共有モデルは派生ごとの static StaticInitialize — 根拠: BaseEffect.h, GameScene.cpp:85-86/378-390/514-516
- **再利用型エフェクト（1 個を Play で再生）** — AttackEffect を Player が値メンバで保持 — 根拠: AttackEffect.cpp:31-37, Player.h:117
- **オブジェクト管理は std::vector + 生ポインタ（std::list / unique_ptr 不使用）** — 根拠: GameScene.h:93-102
- **マップチップ CSV（20 行×100 列・2 文字コード・空白セル=kBlank）** — `B0` ブロック、`S0-S3` 急坂、`G0-G7` 緩坂、`P0/E0/E1` 配置オブジェクト、文字列→enum は std::map テーブル — 根拠: MapChipField.cpp:15-36, 52-90
- **地形と配置オブジェクトを同じ CSV から読み分け** — オブジェクトは MapObjectSpawn（種類・インデックス・ワールド座標）に蓄積、GameScene がスポーン情報から Player/Enemy を生成（位置はハードコードしない） — 根拠: MapChipField.cpp:77-86, GameScene.cpp:184-218
- **表示ブロックは WorldTransform* の 2 次元 vector（空白は nullptr）** — 根拠: GameScene.cpp:112-124, 147-161, 416-427
- **地形種に応じたモデル切替と回転（Y/X/Z 180°で向きを作る）** — 根拠: GameScene.cpp:11-37, 468-493
- **ホットリロード（ImGui ボタンで CSV 再読込・オブジェクト作り直し・カメラ再初期化）** — 根拠: GameScene.cpp:226-237, 331-337
- **ステージ複数化（StageManager）** — `stageDatas.csv`（名前,制限時間）→ `vector<StageData>`、現在番号の管理・名前指定、GameScene が名前から CSV パスを合成 — 根拠: StageManager.cpp:18-71, StageManager.h:16-57, GameScene.cpp:130-135
- **フェード** — 白 1×1 テクスチャのスプライトを画面サイズに拡大し黒・アルファを時間で変化、Start/Stop/IsFinished/GetStatus、シーン開始で FadeIn・終了で FadeOut — 根拠: Fade.cpp, TitleScene.cpp:38/73, GameScene.cpp:107/430-445
- **タイトル画面（3D モデル表示 + サイン波の揺れ + スペースで開始）** — 根拠: TitleScene.cpp:15-95
- **デスアニメーション** — 敵接触: プレイヤー非表示 + DeathParticles（8 方向飛散・フェード）→ 完了後 FadeOut → タイトル；挟まれ死: 飛び上がって落下；敵: 飛び上がり回転フェード — 根拠: GameScene.cpp:348-355/437-445, Player.cpp:521-525/512-517, BaseEnemy.cpp:205-253
- **攻撃判定** — kAttack 中に AABB が重なれば `OnHitByAttack(方向)`、通常時の接触はプレイヤーがやられる、空中攻撃は 1 回、突進中は壁で停止 — 根拠: GameScene.cpp:524-566, Player.cpp:361-384, 425-432
- **ガード（向かい合い判定 `attackDirection * facing < 0`）→ のけぞり演出 + プレイヤーのノックバック + 中点に GuardEffect** — 根拠: ShieldEnemy.cpp:49-96, GameScene.cpp:543-553
- **ヒットエフェクト（円 + ランダム回転トゲ 2 本、拡大→フェードの 2 フェーズ）** — 根拠: HitEffect.cpp:62-154
- **半透明の描画順序** — 不透明（地形・天球）→ 半透明（攻撃板・パーティクル）→ 深度テスト OFF の別バッチで最前面にエフェクト → フェード — 根拠: GameScene.cpp:448-521, Player.cpp:530-534
- **GlobalVariables（調整項目の一元管理）** — シングルトン + `map<グループ, map<項目, variant>>`、各クラスが static Register（AddItem は読み込み済みを上書きしない）/ Apply（毎フレーム反映）、ImGui メニューバーで DragInt/DragFloat/DragFloat3 編集、Save ボタンで JSON 保存、起動時 LoadFiles で全 JSON 読込、Rect は 4 つの float 項目に分解 — 根拠: GlobalVariables.cpp, main.cpp:135-154, GameScene.cpp:292-299/312-314, CameraController.cpp:23-27
- **ImGui デバッグ UI** — 敵の生存数表示 + Respawn All、マップのリロード、デバッグ時 S キーで Player.json 保存 — 根拠: GameScene.cpp:306-337, 570-575
- **デバッグカメラ切替** — Enter で ON/OFF、ON 時は DebugCamera の matView/matProjection をコピーして TransferMatrix — 根拠: GameScene.cpp:301-304, 395-413
- **天球（Skydome クラス）** — 根拠: Skydome.cpp:4-19
- **早期 return と NULL チェック assert** — `if (isFinished_) return;`、`assert(model)`、`assert(mapChipField_)` — 根拠: AttackEffect.cpp:43-45, Player.cpp:95/539
- **1 フレームに同じ WorldTransform を複数回描かない前提（配列で個別に持つ）** — パーティクルは個数分の WorldTransform — 根拠: DeathParticles.h:54, HitEffect.h:76

---

## D. 使用している KamataEngine API（移植対応表の原材料）

### D-1. エンジン全体 / DirectXCommon / WinApp / ImGuiManager

- **`#include "KamataEngine.h"`** — 統合ヘッダ（個別ヘッダは `<math/Vector3.h>` のみ使用） — 根拠: Player.h:4, GlobalVariables.h:7
- **`KamataEngine::Initialize(L"LE2A_12_スズキ_ダイスケ_AL3")`** — エンジン初期化・ウィンドウタイトル — 根拠: main.cpp:128
- **`KamataEngine::Update()`** — 毎フレームのエンジン更新、true なら終了 — 根拠: main.cpp:175
- **`KamataEngine::Finalize()`** — 終了処理 — 根拠: main.cpp:221
- **`DirectXCommon::GetInstance()`** — シングルトン取得 — 根拠: main.cpp:129, Fade.cpp:56
- **`dxCommon->PreDraw()` / `PostDraw()`** — 描画開始・終了（シーン描画を挟む） — 根拠: main.cpp:196, 207
- **`DirectXCommon::GetCommandList()`** — Sprite::PreDraw に渡す — 根拠: Fade.cpp:56
- **`WinApp::kWindowWidth` / `kWindowHeight`** — 画面サイズ定数（フェード用スプライトのサイズ） — 根拠: Fade.cpp:11
- **`ImGuiManager::GetInstance()->Begin()` / `End()` / `Draw()`** — ImGui フレーム開始（Update 前）・確定（Update 後）・描画（PostDraw 前）、`#ifdef USE_IMGUI` で囲む — 根拠: main.cpp:182-204

### D-2. Model

- **`Model::CreateFromOBJ("player", true)`** — プレイヤーモデル読込（タイトルでも使用） — 根拠: GameScene.cpp:55, TitleScene.cpp:20
- **`Model::CreateFromOBJ("titleFont", true)`** — タイトル文字モデル — 根拠: TitleScene.cpp:21
- **`Model::CreateFromOBJ("skydome", true)`** — 天球 — 根拠: GameScene.cpp:60, Skydome.cpp:7
- **`Model::CreateFromOBJ("enemy", true)` / `("shieldEnemy", true)`** — 通常の敵・盾持ちの敵 — 根拠: GameScene.cpp:61-62
- **`Model::CreateFromOBJ("deathParticle", true)`** — 白い球（デスパーティクルとヒットエフェクトで共用） — 根拠: GameScene.cpp:63, 85
- **`Model::CreateFromOBJ("ring", true)`** — ガードエフェクトの白い輪 — 根拠: GameScene.cpp:64, 86
- **`Model::CreateFromOBJ("hit_effect", true)`** — 体当たりエフェクトの板 — 根拠: GameScene.cpp:65, 192
- **`Model::CreateFromOBJ("slopeSteep" / "slopeGentleLow" / "slopeGentleHigh", false)`** — 坂モデル（第 2 引数だけ false。引数の意味は本レポートでは未確認） — 根拠: GameScene.cpp:57-59
- **`Model::Create()`** — 既定の立方体（ブロック用） — 根拠: GameScene.cpp:56
- **`Model::PreDraw()`** — 3D 描画前処理（既定引数） — 根拠: TitleScene.cpp:88
- **`Model::PreDraw(Model::CullingMode::kNone)`** — カリングなし（坂モデルの裏面対策） — 根拠: GameScene.cpp:451
- **`Model::PreDraw(Model::CullingMode::kNone, Model::BlendMode::kNormal, Model::DepthTestMode::kOff)`** — 深度テストなしの最前面バッチ — 根拠: GameScene.cpp:512
- **`Model::PostDraw()`** — 3D 描画後処理 — 根拠: GameScene.cpp:508/517, TitleScene.cpp:91
- **`model->Draw(worldTransform, camera)`** — 通常描画（WorldTransform / Camera は参照渡し） — 根拠: Player.cpp:527, GameScene.cpp:493, Skydome.cpp:18, TitleScene.cpp:89-90, BaseEnemy.cpp:265
- **`model->Draw(worldTransform, camera, &objectColor)`** — 色・アルファ付き描画（フェード演出） — 根拠: BaseEnemy.cpp:261, DeathParticles.cpp:101, HitEffect.cpp:152, GuardEffect.cpp:97, AttackEffect.cpp:77
- **`delete model`** — モデルの解放は生成者が delete — 根拠: GameScene.cpp:244/263-272, TitleScene.cpp:11-12

### D-3. WorldTransform

- **`WorldTransform` を値メンバ / `new WorldTransform()`** — 根拠: Player.h:14, GameScene.cpp:155, DeathParticles.h:54
- **`worldTransform_.Initialize()`** — 定数バッファ等の初期化（必ず最初に呼ぶ） — 根拠: Player.cpp:102, GameScene.cpp:81/156
- **`translation_` / `rotation_` / `scale_`（公開メンバ）** — 位置・回転（ラジアン）・拡大を直接代入 — 根拠: Player.cpp:103-105, TitleScene.cpp:26-28
- **`matWorld_ = MakeAffineMatrix(…)`** — 自作行列を公開メンバに代入（エンジンの行列更新関数は未使用） — 根拠: Player.cpp:150, GameScene.cpp:422
- **`TransferMatrix()`** — 定数バッファへ転送 — 根拠: Player.cpp:152, GameScene.cpp:425, HitEffect.cpp:140

### D-4. Camera / DebugCamera

- **`Camera` を値メンバで所有し、オブジェクトへは `Camera*` を渡す** — 根拠: GameScene.h:80, TitleScene.h:42, Player.h:18
- **`camera_.Initialize()`** — 既定位置 {0,0,-50} から +Z 向き（コメントより） — 根拠: GameScene.cpp:69, TitleScene.cpp:16-17
- **`camera_.UpdateMatrix()`** — ビュー・射影の更新と転送 — 根拠: GameScene.cpp:412, TitleScene.cpp:57
- **`camera_.TransferMatrix()`** — 行列を直接書き換えた後の転送（デバッグカメラ時） — 根拠: GameScene.cpp:407
- **`camera_.matView` / `camera_.matProjection`（公開メンバ）** — デバッグカメラの行列をコピー — 根拠: GameScene.cpp:402-404
- **`camera_->translation_`** — カメラ位置を直接操作（追従・スクロール・画面端計算） — 根拠: CameraController.cpp:73-86/144-150, Player.cpp:483-492
- **`camera_->fovAngleY` / `camera_->aspectRatio`** — 可視範囲の算出 — 根拠: Player.cpp:484-485, CameraController.cpp:171-172
- **`new DebugCamera(1280, 720)`** — 生成（画面サイズ直書き） — 根拠: GameScene.cpp:70
- **`debugCamera_->Update()`** — デバッグカメラ操作の更新 — 根拠: GameScene.cpp:397
- **`debugCamera_->GetCamera()`** — `const Camera&` を取得して view/proj をコピー — 根拠: GameScene.cpp:400

### D-5. ObjectColor / 数学型

- **`ObjectColor objectColor_` + `Initialize()` + `SetColor(Vector4)`** — アルファでフェードアウト — 根拠: BaseEffect.h:43, BaseEnemy.cpp:21/45/243, DeathParticles.cpp:47-49/84
- **`KamataEngine::Vector3`（x,y,z 公開メンバ、波括弧初期化、`Vector3(a,b,c)` 丸括弧生成）** — 根拠: Player.h:20, MapChipField.cpp:105-109
- **`KamataEngine::Vector4`（w をアルファに使用）** — 根拠: BaseEffect.h:45, BaseEnemy.cpp:242
- **`KamataEngine::Matrix4x4`（`.m[4][4]` 公開配列）** — 自作関数で読み書き — 根拠: Matrix4x4.cpp:12, 32-34
- **エンジンの MathUtility（演算子等）は未使用** — Vector3 の演算は手書き — 根拠: Player.cpp:90（grep で `MathUtility` 該当なし）

### D-6. Sprite / TextureManager

- **`TextureManager::Load("white1x1.png")`** — テクスチャハンドル（uint32_t）取得。フェード用 — 根拠: Fade.cpp:8, Fade.h:55
- **`TextureManager::Load("uvChecker.png")`** — GameScene で読込（スプライトは生成のみで描画していない） — 根拠: GameScene.cpp:50-51
- **`Sprite::Create(textureHandle, {x, y})`** — スプライト生成 — 根拠: Fade.cpp:9, GameScene.cpp:51
- **`sprite_->SetSize({w, h})`** — 画面全体に拡大 — 根拠: Fade.cpp:11
- **`sprite_->SetColor({r, g, b, a})`** — 黒 + アルファ変化 — 根拠: Fade.cpp:13/36/45
- **`Sprite::PreDraw(commandList)` → `sprite_->Draw()` → `Sprite::PostDraw()`** — 2D 描画の括り（3D の後に描いて最前面） — 根拠: Fade.cpp:56-58

### D-7. Input

- **`Input::GetInstance()->PushKey(DIK_RIGHT / DIK_LEFT)`** — 左右移動（押下継続） — 根拠: Player.cpp:165-184
- **`PushKey(DIK_UP)`** — 可変ジャンプ（押し続け判定） — 根拠: Player.cpp:251
- **`TriggerKey(DIK_UP)`** — ジャンプ（押した瞬間） — 根拠: Player.cpp:218
- **`TriggerKey(DIK_SPACE)`** — 攻撃開始 / タイトルからゲームへ — 根拠: Player.cpp:231, TitleScene.cpp:72
- **`TriggerKey(DIK_RETURN)`** — デバッグカメラ切替（_DEBUG） — 根拠: GameScene.cpp:302
- **`TriggerKey(DIK_S)`** — Player.json 保存（_DEBUG） — 根拠: GameScene.cpp:307
- **DIK_* 定数（DirectInput キーコード）のみ使用。マウス・ゲームパッドは未使用** — 根拠: grep（`GetJoystickState`/`GetMouse` 該当なし）

### D-8. ImGui（直接呼び出し）

- **`#include <imgui.h>`（USE_IMGUI 時のみ）** — 根拠: GlobalVariables.cpp:14-16
- **`ImGui::Begin("Enemy") … ImGui::End()`、`ImGui::Begin("Map")`** — デバッグウィンドウ — 根拠: GameScene.cpp:317-337
- **`ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar)` + `BeginMenuBar/EndMenuBar` + `BeginMenu/EndMenu`** — グループごとのメニュー — 根拠: GlobalVariables.cpp:272-329
- **`ImGui::Text(fmt, …)`** — 生存数・ステージ名表示 — 根拠: GameScene.cpp:324/332-333
- **`ImGui::Button("…")`** — Respawn All / Reload stage CSV / Save — 根拠: GameScene.cpp:325/334, GlobalVariables.cpp:319
- **`ImGui::DragInt` / `DragFloat` / `DragFloat3`** — variant の型ごとに編集 UI を出し分け — 根拠: GlobalVariables.cpp:302-312
- **imgui.ini をリポジトリに含めている（ウィンドウ位置）** — 根拠: imgui.ini:1-19

### D-9. 未使用の KamataEngine 機能（ex3 時点）

- **Audio / PrimitiveDrawer / AxisIndicator / MathUtility / Input のパッド・マウス** — 学生コードに出現しない（Resources の fanfare.wav / mokugyo.wav / axis / cube / particle / sample.png / tex1.png / debugfont.png はエンジン付属資材で未参照） — 根拠: 全ファイル grep、Resources 一覧

---

## E. 判断に迷うもの・備考

- **ルートの `Vector3.h`（独自 struct）は未使用** — AL3_05_04 で追加されたが include 箇所がない（GlobalVariables.h が include するのはエンジンの `<math/Vector3.h>`） — 根拠: Vector3.h:1-4, GlobalVariables.h:7, `git grep Vector3.h`
- **`Matrix4x4.h` は型ではなく関数群** — エンジンの Matrix4x4 型に対する自作関数を `namespace KamataEngine` 内に定義。移植対象は「S·R·T の順序と行優先規約」 — 根拠: Matrix4x4.h:4-21
- **GameScene.h に行列関数宣言のコメントアウト残骸** — Matrix4x4.h へ移動済み — 根拠: GameScene.h:20-39
- **未使用メンバ/値** — GameScene の `sprite_`/`textureHandle_`（uvChecker、描画なし）と `worldTransform_`（Initialize のみ）、`Player::kBounceVelocity`（登録されるが未使用）、`StageData::timeLimit`（読込のみ） — 根拠: GameScene.cpp:50-51/81, Player.h:86, StageManager.h:10
- **Fade は `sprite_` を delete していない（デストラクタなし）** — 根拠: Fade.h:7-66
- **エフェクトの 2 方式が混在** — 再利用型 AttackEffect（BaseEffect 非継承・Player が所有）と使い捨て型 HitEffect/GuardEffect（BaseEffect 継承・GameScene のリスト） — 根拠: AttackEffect.h:8, HitEffect.h:15
- **`.obj` が gitignore（`*.obj`）のためモデル本体はリポジトリに無い** — Resources には .mtl と .png のみ（モデル名だけ把握可能） — 根拠: .gitignore:78, `find Resources -name "*.obj"` = 0 件
- **External（KamataEngine / imgui / nlohmann / DirectXTex）はリポジトリ外** — `$(ProjectDir)..\External` 参照、このワークツリーでは存在せずビルド不可 — 根拠: DirectXGame.vcxproj:62-63
- **タグとコミットの対応の注意** — `AL3_05_09` と `AL3_05_10` は同一のマージコミット 4c19a78（AABB.h・Enemy 追加）、`AL3_05_12` と `AL3_05_13` は同一コミット 82c853e、`AL3_05_08` は時系列では 05_13 の後（2a2f4b8）、`AL3_05_18` は develop のマージ 5b7f270 — 根拠: `git log --oneline --decorate AL3_05_19_ex3`
- **`_ex` 付きタグは発展内容** — 05_07_ex 強制スクロール、05_17_ex BaseEffect 統合、05_19_ex1〜3 GlobalVariables（ImGui 編集→JSON 保存→JSON 読込）。それ以外の「調整」コミット（0bca3dd, 0b2108b, 0bf3b88, 7d4c77d）の内容（坂地形・コヨーテタイム・サブステップ・FollowAxis の慣性等）が授業由来か自前拡張かはコミットメッセージからは判別できない — 根拠: `git log`
- **時間はフレーム基準（1/60 固定）でデルタタイム無し、速度は「1 フレームあたりの移動量」単位** — 根拠: Player.h:94 コメント, Fade.cpp:31
- **unique_ptr は授業コードに登場しない（raw new/delete）** — 自作エンジン側の方針（unique_ptr 推奨）とは異なるが、知識としては所有権の考え方自体は学んでいる — 根拠: GameScene.cpp:241-287
- **Camera = 旧 ViewProjection 相当** — `matView/matProjection/TransferMatrix/UpdateMatrix/fovAngleY/aspectRatio/translation_` を持つ — 根拠: GameScene.cpp:400-412, Player.cpp:483-485
- **CSV の制限時間は読み込むだけで未使用、ゴールも存在しない（ex3 時点ではクリア条件なし）** — 根拠: StageManager.h:10, GameScene.cpp 全体

---

## F. タグ（章）ごとの学習トピック一覧

| タグ | コミット | メッセージ | 追加/変更された学生ソース（抜粋） |
|---|---|---|---|
| (前) | d93839a, 3a85c6a | .gitattributes/.gitignore、プロジェクトファイル追加 | main.cpp |
| AL3_04_02 | 6062902 | KAMATA ENGINE を実行可能な状態にした | GameScene.h/.cpp 追加、main.cpp |
| AL3_05_01 | d7c2cc0 | 自キャラをクラス化して 3D オブジェクトを表示した | Player.h/.cpp 追加（Model / WorldTransform / Camera） |
| AL3_05_02 | 7d3f115 | 05_02 を初回コミット | Matrix4x4.h/.cpp 追加（自作アフィン行列）、GameScene |
| AL3_05_03 | 0dcfd3c, 564f399 | 天球を実装した（+誤字修正） | Skydome.h/.cpp 追加 |
| AL3_05_04 | a20f680 | 外部ファイルからマップチップデータを読み込んで配置した | MapChipField.h/.cpp 追加、Resources/block.csv、Vector3.h |
| AL3_05_05 | 2c215cd | プレイヤーの左右移動とジャンプを実装 | Easing.h/.cpp 追加、Player（加速減速・旋回イージング・重力・ジャンプ） |
| AL3_05_06 | d683631 | 追従カメラを実装 | CameraController.h/.cpp 追加 |
| AL3_05_07 | 057e0fc | プレイヤーとマップチップとの当たり判定を実装 | Player / MapChipField（IndexSet・Rect・4 隅） |
| AL3_05_07_ex | 190eee9 | 強制スクロールを実装 | CameraController（Mode::kScroll）、Player（画面内制限） |
| — | 0bca3dd | プレイヤーの壁に対する処理を微調整 | Player.cpp |
| AL3_05_09 / 05_10 | 4c19a78（マージ） | 敵を実装 | Enemy.h/.cpp・AABB.h 追加、CameraController/GameScene/Player |
| AL3_05_11 | b3fb2bb | 敵に当たってやられたときの演出を追加 | DeathParticles.h/.cpp 追加、Player のやられ状態 |
| AL3_05_12 / 05_13 | 82c853e | フェーズやシーン遷移の仕組みを実装 | Fade.h/.cpp・TitleScene.h/.cpp 追加、main.cpp のシーン切替、GameScene の Phase |
| AL3_05_08 | 2a2f4b8 | 当たり判定を少し修正 | Player.cpp |
| AL3_05_14 | 1e79071 | プレイヤーの攻撃動作を実装 | HitEffect.h/.cpp 追加、Player の Behavior::kAttack |
| AL3_05_15 | e9961cb | 敵がプレイヤーの攻撃に当たったときの振る舞いを追加 | Enemy のやられ演出（ObjectColor フェード）、main.cpp |
| — | 0b2108b, 0bf3b88 | 攻撃のヒットエフェクト実装 / 攻撃に関する修正 | SlashEffect 追加→削除、AttackEffect.h/.cpp 追加 |
| AL3_05_16 | 7d4c77d | さらに調整 | AttackEffect / HitEffect / Player / GameScene |
| AL3_05_17 | 9158fa1 | 正面からは倒せない敵を実装 | BaseEnemy.h/.cpp・ShieldEnemy.h/.cpp・GuardEffect.h/.cpp 追加（基底クラス化） |
| AL3_05_17_ex | 2530739 | 各エフェクトの共通処理を BaseEffect から継承させてリスト統合 | BaseEffect.h 追加、GameScene の effects_ |
| AL3_05_18 | 5b7f270（850246d のマージ） | マップチップの配置方法を変更 | MapChipField（2 文字コード、P0/E0/E1 配置）、GameScene の LoadLevel |
| AL3_05_19 | 083821b | 複数のステージデータを扱えるように | StageManager.h/.cpp 追加、stageDatas.csv・field01〜03.csv、main.cpp |
| AL3_05_19_ex1 | 509a6b1 | データを一か所にまとめて編集可能に | GlobalVariables.h/.cpp 追加（ImGui 編集） |
| — | ab313ad | ImGui で調整した数値を Save ボタンから JSON 保存 | GlobalVariables（SaveFile） |
| AL3_05_19_ex2 | 9f7e946 | 少し調整 | Resources/GlobalVariables/Player.json 追加 |
| AL3_05_19_ex3 | ea88269 | JSON ファイルから調整項目を読み込めるように | GlobalVariables（LoadFiles/LoadFile）、全クラスの Register/Apply、全 JSON |

根拠: `git log --oneline --decorate AL3_05_19_ex3`、`git log --reverse --name-status AL3_05_19_ex3 -- '*.h' '*.cpp'`。

---

## G. ex3 以降（知識としては範囲外、参考用）

### G-1. コミット（`git log --oneline AL3_05_19_ex3..master`、古い順）

- 96b122f デバッグ用に重複した機能の片方を削除して統合
- f4feb51 ステージを比較的手軽にエディットできる機能（MapEditor 追加）＋地形との当たり判定修正
- f14d899 地形が自然に描画されるようにした
- 14168b5 Xbox コントローラーによる操作に対応（GameInput 追加）
- 7fa2f83 箱を実装（Crate / TerrainPhysics 追加）
- e2906f0 ワールドマップからステージを選択（WorldMapScene / SaveData / Goal / PhotoSystem 追加）
- 66bce78 プレイヤー自身を強化できる要素
- acda644 様々な箇所にエフェクトを追加（SpriteEffect / PlayerModel / TutorialGuide 追加）
- 8ceae51 チュートリアルの文章を作成
- a976a1c ステージのクリア演出（PlayerMotion 追加）
- 6a414d4 ゲームとして遊べる必要な機能（BackgroundScenery / BossEnemy / MenuCursor / SoundManager 追加）
- dce8f0a Ver.1.0 タイトル画面の作り込み、箱の描画バグ修正、効果音追加
- 6db312d develop（Ver.1.0）を master へ統合

（ファイル→コミットの対応は `git log --diff-filter=A -- <file>` で確認。）

### G-2. ex3 以降に追加された新規クラス（ヘッダ先頭コメントより）

- **BackgroundScenery** — 遠景（地面と木）の視差付き背景、ImGui で木を配置し JSON 保存
- **BossEnemy : BaseEnemy** — 大型ボス。体当たりはガード、落下する箱が弱点、HP 制
- **Crate** — 木箱。重力・坂で滑る・押せる・乗れる、速い時は敵へのダメージ源
- **GameInput** — 論理アクション入力層（キーボード + XInput パッド併用、トリガー判定自前）
- **Goal** — ゴールの旗モデル。触れるとクリア
- **MapEditor** — デバッグ中のマップ編集（マウス配置・スポイト・Undo・範囲選択・Ctrl+S で CSV 保存）
- **MenuCursor** — メニュー選択カーソル（回転する三角形スプライト）
- **PhotoSystem** — 写真システム（写ったオブジェクトのスナップショットと複製管理、ヘッダオンリー）
- **PlayerModel** — パーツ分割式プレイヤーモデル（親子行列合成、表情パーツ）
- **PlayerMotion**（Pose / Clip / Library / Player / Editor） — 自前キーフレームアニメ + ImGui モーションエディタ
- **SaveData** — セーブデータ（ステージ解放・強化レベル）を JSON で永続化
- **SoundManager** — エンジン Audio を包む BGM/SE 管理、GlobalVariables で音量調整
- **SpriteEffect : BaseEffect** — 画像差し替え式の汎用板エフェクト
- **TerrainPhysics** — 地形に対する簡易 2D 物理の共通関数（任意の矩形エンティティ用）
- **TutorialGuide** — 画像化した文章を状況に応じて順番に表示するチュートリアル
- **WorldMapScene** — ワールドマップシーン（スティックでカーソル移動、ステージ選択）

### G-3. ex3 以降に初めて使われたエンジン API・資材

- **Audio**（`LoadWave` / `PlayWave` / `StopWave` / `SetVolume`）、**Input::GetJoystickState**（XInput）、**MathUtility**（10 箇所）、**TextureManager::Load** の多用（70 箇所。UI・エフェクト画像） — 根拠: `git grep` on master
- Resources は `Resources/GameData/...` 配下へ移動（GlobalVariables / stageDatas / sounds / effects / fonts / motions / background） — 根拠: `git diff --name-status AL3_05_19_ex3 master -- Resources`
- 規模: 56 ファイル、+14,930 / −443 行（GameScene.cpp だけで +4,453） — 根拠: `git diff --stat`

---

## H. このソースで学んだと言える主要トピック（タグ順）

1. **AL3_04_02** — KamataEngine の起動・メインループ（Initialize / Update / PreDraw / PostDraw / Finalize）と GameScene の Initialize/Update/Draw 骨格。
2. **AL3_05_01** — 自キャラのクラス化: Model::CreateFromOBJ、WorldTransform、Camera をポインタで受け取る Initialize(model, position, camera) 型の設計。
3. **AL3_05_02** — 4x4 行列関数の自作（平行移動・拡縮・回転・アフィン合成 S·R·T）と matWorld_ への代入 + TransferMatrix。
4. **AL3_05_03** — 天球クラス（常に描画される背景モデル）。
5. **AL3_05_04** — CSV からのマップチップ読込（ifstream → stringstream → getline(',')）、2 次元 vector、インデックス→ワールド座標変換、ブロックの大量配置。
6. **AL3_05_05** — 左右移動（加速・摩擦・最高速）、旋回アニメ（lerp + EaseInOut）、重力とジャンプ、enum class による向き管理。
7. **AL3_05_06** — 追従カメラ（オフセット・補間・速度先読み・マージン・マップ端の移動可能範囲）。
8. **AL3_05_07** — マップチップとの当たり判定（座標→インデックス、セル矩形、4 隅、方向別の判定と押し戻し、接地判定）。
9. **AL3_05_07_ex** — 強制スクロールと画面内制限（画角から画面端を算出、挟まれ死）。
10. **AL3_05_09/10** — 敵クラス（等速歩行・壁で反転・歩行揺れ）と AABB 同士の当たり判定、プレイヤー⇔敵の衝突コールバック。
11. **AL3_05_11** — やられ演出: モデルパーティクル（8 方向飛散、ObjectColor でアルファフェード）、プレイヤーの死亡状態。
12. **AL3_05_12/13** — シーン遷移（enum + switch、IsFinished）、フェーズ管理、フェード（スプライト + アルファ）、タイトルシーン。
13. **AL3_05_14** — 攻撃アクション: Behavior ステートマシン（optional による切替要求）、3 フェーズの形状イージング、攻撃判定の有効化。
14. **AL3_05_15** — 攻撃を受けた敵の振る舞い（やられ演出・当たり判定の無効化）。
15. **AL3_05_16 周辺** — ヒットエフェクト（再利用型 AttackEffect / 使い捨て HitEffect、半透明の描画順・深度テスト OFF バッチ）。
16. **AL3_05_17** — 敵の基底クラス化と派生（仮想関数のオーバーライド、HitResult の返却、ガードとのけぞり、ノックバック、GuardEffect）。
17. **AL3_05_17_ex** — エフェクトの基底クラス（純粋仮想）と統合リスト（生成 → 終了で delete + erase）、static な共有リソース。
18. **AL3_05_18** — マップチップの 2 文字コード化と、CSV からのオブジェクト配置（P0/E0/E1 → スポーン情報 → 生成）、地形種によるモデル切替、ホットリロード。
19. **AL3_05_19** — ステージ複数化（ステージ一覧 CSV、StageManager、名前からのパス合成、デバッグ用開始ステージ指定）。
20. **AL3_05_19_ex1** — GlobalVariables（シングルトン・map + variant・ImGui メニューで編集・各クラスの Register/Apply）。
21. **AL3_05_19_ex2〜ex3** — nlohmann::json による保存（Save ボタン・filesystem でディレクトリ作成）と起動時の一括読込（directory_iterator）、読み込み値を既定値で上書きしない AddItem。

---

## 付録: Resources の構成（ex3 時点）

- **モデル（フォルダ名 = CreateFromOBJ の引数。.obj は gitignore で欠落、.mtl/.png のみ）**: player, titleFont, SkyDome(skydome), enemy, shieldEnemy, deathParticle(white1x1.png), ring, hit_effect, slopeSteep, slopeGentleLow, slopeGentleHigh, block(block.png: Model::Create 用), axis / cube / particle（エンジン付属・未使用）
- **テクスチャ**: white1x1.png（フェード）, uvChecker.png（未描画スプライト）, sample.png / tex1.png / debugfont.png / kamata.ico（エンジン付属）
- **音**: fanfare.wav, mokugyo.wav（エンジン付属・未使用）
- **シェーダ**: Resources/shaders/*.hlsl(i)（エンジン付属）
- **ステージ**: `Resources/stageDatas/stageDatas.csv` = `name,timeLimit` 行（field01,100 / field02,130 / field03,50）；`fieldNN.csv` = 20 行 × 100 列、セルは空 / `B0` / `S0-S3` / `G0-G7` / `P0` / `E0` / `E1`
- **調整項目 JSON**: `Resources/GlobalVariables/<Group>.json` = `{"Group": {"Item": 数値 | [x, y, z]}}`（Player 31 項目、Enemy 13、CameraController 10、HitEffect 6、GuardEffect 3、DeathParticles 3、ShieldEnemy 2）
- **imgui.ini**: ウィンドウ "Enemy" / "Map" / "Global Variables" の位置

---

## コーディングスタイル（AL3 学生コードの観察）

観察した事実のみ。根拠はファイル:行（worktree `AL3_ex3`）または集計コマンドの結果。

### 命名

- **クラス名** — PascalCase の英語名詞。役割接尾辞: `…Scene`, `…Manager`, `…Controller`, `…Effect`, `…Particles`, `Base…`（基底） — 根拠: GameScene.h:42, StageManager.h:16, CameraController.h:11, GuardEffect.h:12, DeathParticles.h:9, BaseEnemy.h:11
- **struct 名** — PascalCase（AABB, StageData, TerrainShape, IndexSet, Rect, MapObjectSpawn, CollisionMapInfo） — 根拠: AABB.h:7, StageManager.h:8, MapChipField.h:30/60/66/52, Player.h:152
- **class のメンバ変数** — camelCase + 末尾 `_`（`worldTransform_`, `model_`, `velocity_`, `lrDirection_`, `isFinished_`） — 根拠: Player.h:14-20, BaseEffect.h:47
- **struct のメンバ** — 末尾 `_` なし（`min/max`, `left/right`, `name/timeLimit`, `hasFloor`） — 根拠: AABB.h:8-9, MapChipField.h:67-70, StageManager.h:9-10
- **static 共有メンバ** — `s` 接頭辞 + 末尾 `_`（`sModel_`, `sCamera_`） — 根拠: HitEffect.h:71-73, GuardEffect.h:52-54
- **定数** — `k` 接頭辞 + PascalCase（`kAcceleration`, `kNumParticles`, `kBlockWidth`, `kFadeDuration`, 名前空間定数 `kDirectoryPath`） — 根拠: Player.h:22, DeathParticles.h:41, MapChipField.h:107, GameScene.h:117, GlobalVariables.cpp:24
- **定数の宣言形** — 調整可能値は `static inline float kX = …;`（非 const）、固定値は `static inline const float/uint32_t kX`、ローカルは `const float`。`constexpr` は 0 件 — 根拠: Player.h:22, AttackEffect.h:46, GameScene.cpp:12、grep 集計
- **enum / 列挙子** — `enum class` + 列挙子は `k` 接頭辞 PascalCase（`kFadeIn`, `kRoot`, `kBlank`）、末尾カンマあり、各列挙子に行末コメント。要素数番兵は `kNumCorner`、明示値は `kUnknown = 0` のみ — 根拠: Fade.h:10-14, Player.h:34-39, Player.h:148, main.cpp:14-18
- **関数名** — PascalCase、動詞始まり: `Initialize/Update/Draw`, `Get…/Set…/Is…/On…/Try…/Check…/Calc…/Find…/Load…/Save…/Create…/Make…/Reset/Respawn/Play/Start/Stop` — 根拠: Player.h:160-279, MapChipField.h:78-103, Matrix4x4.h:6-20
- **bool 名** — `is…` / `has…` / `on…` / 過去分詞（`isFinished_`, `hasFloor`, `onGround_`, `finished_`, `guarding_`, `hit`, `found`, `land`） — 根拠: AttackEffect.h:65, Player.cpp:618, Player.h:76, TitleScene.h:60, ShieldEnemy.h:36, Player.cpp:773/688/636
- **ローカル変数** — camelCase（`aliveCount`, `stageFileName`, `halfViewWidth`）、短い数学変数 `t`, `s`, `h`, `c`, `i/j/x/y`、イテレータ `it` — 根拠: GameScene.cpp:318/132, CameraController.cpp:172, HitEffect.cpp:109, GuardEffect.cpp:79, Matrix4x4.cpp:50, GameScene.cpp:383
- **引数名** — camelCase、出力引数は `out` 接頭辞（`outSurfaceY`）、未使用引数は名前をコメントアウト（`float /*attackDirection*/`） — 根拠: MapChipField.h:93, BaseEnemy.cpp:205
- **ポインタ名** — 接頭辞なし（`model_`, `titleScene`, `dxCommon`） — 根拠: Player.h:16, main.cpp:24, main.cpp:129
- **言語** — 識別子は全て英語（ローマ字なし）、コメントは日本語。略語: `lr`（LRDirection）, `dx`（dxCommon）, `mat`（matWorld_）, `Pos`, `Calc`, `Num`, `Csv`, `Obj`, `Vec3`（なし: `Vector3` と書く） — 根拠: Player.h:28-32, main.cpp:129, GameScene.cpp:547, Player.h:191, MapChipField.h:81/111
- **ファイル名** — クラス名と同じ PascalCase の .h/.cpp を 1 クラス 1 組（例外: BaseEffect.h・Vector3.h・AABB.h・Easing はヘッダ中心） — 根拠: ディレクトリ一覧

### ファイル構成

- **ヘッダの並び** — `#pragma once` → 自作ヘッダ `"…"`（アルファベット順）→ 標準ヘッダ `<…>` → 前方宣言 → コメント付きクラス。古い GameScene.h だけ include が依存順 — 根拠: Player.h:1-6, HitEffect.h:1-5, GameScene.h:1-18
- **.cpp の並び** — （必要なら `#define NOMINMAX`）→ 自分のヘッダ → 他の自作ヘッダ → 標準ヘッダ（アルファベット順）→ `using namespace KamataEngine;` → 無名名前空間 → 関数定義 — 根拠: Player.cpp:1-11, BaseEnemy.cpp:1-8, GlobalVariables.cpp:1-25
- **`using namespace` はヘッダに書かない** — ヘッダは `KamataEngine::Vector3` と完全修飾 — 根拠: Player.h:14-20, Fade.h:57
- **include ガード** — `#pragma once` のみ（`#ifndef` ガードなし） — 根拠: 全ヘッダ 1 行目
- **前方宣言の活用** — 依存を軽くする目的でコメント付き — 根拠: GameScene.h:17-18, CameraController.h:4-6
- **クラス内の並び（2 通りが混在）** — 旧クラスは `private:` メンバ → `public:` 関数（GameScene.h:43/119, Player.h:12/197, Skydome.h:8/16）、新しいクラスは `public:` 関数 → `protected:` → `private:` メンバ（BaseEnemy.h:12/82, Fade.h:8/53, HitEffect.h:16/47, CameraController.h:12/60, StageManager.h:17/52）
- **メンバ関数の順序** — ctor/dtor → Initialize → Update → Draw → getter/setter/判定 → 調整項目の Register/Apply（static）→ private ヘルパ — 根拠: TitleScene.h:10-31, HitEffect.h:23-45, BaseEnemy.h:24-107
- **Initialize のシグネチャ** — `Initialize()`（Fade/TitleScene）、`Initialize(Camera*)`（Skydome）、`Initialize(Model*, const Vector3&, Camera*)`（Player/BaseEnemy/DeathParticles）、`Initialize(Model*, Camera*)`（AttackEffect）、`Initialize(const Vector3&)`（BaseEffect）、`Initialize(StageManager*)`（GameScene）、`Initialize(Camera*, const Player*, MapChipField*)`（CameraController）。`Update()` / `Draw()` は引数なし（カメラはメンバに保持） — 根拠: Fade.h:19, Skydome.h:20, Player.h:204, AttackEffect.h:15, BaseEffect.h:24, GameScene.h:124, CameraController.h:33
- **getter / setter** — ヘッダ内 1 行定義。`const T& GetX() const { return x_; }`、小さい型は値返し `float GetAttackDirection() const`、setter は `void SetX(T* x) { x_ = x; }`、判定は `bool IsX() const` — 根拠: Player.h:229/254/234/244, CameraController.h:48-53
- **.cpp 側の関数の並び** — ヘッダの宣言順ではなく「Register/Apply → 無名名前空間ヘルパ → Initialize → Update → 状態別 Update → 補助 → Draw → 当たり判定」の機能順、`#pragma region` で区切る — 根拠: Player.cpp:13/88/93/109/156/521/536/867
- **ドキュメントコメント** — クラスと public 関数は `/// <summary>…</summary>`（`<param>` `<returns>` も）、メンバ変数と private 関数は `//` の 1 行コメント。GameScene.h は public 関数も `//` のみ — 根拠: Fade.h:4-6/16-18, Player.h:159-195, GameScene.h:123-148, StageManager.h:21-29

### 設計の癖

- **struct と class の使い分け** — 振る舞いを持たないデータの束は struct（全メンバ public、関数なし）、振る舞いを持つものは class — 根拠: MapChipField.h:30-71, StageManager.h:8-11, Player.h:11
- **所有** — 生ポインタ + `new` / `delete`、`std::unique_ptr` は 0 件。生成した側（GameScene / main / TitleScene）がデストラクタまたは作り直し時に delete、受け取る側（Player 等）は delete しない — 根拠: GameScene.cpp:155/241-287, main.cpp:211-218, TitleScene.cpp:9-13、grep 集計
- **値メンバとコンテナ** — 固定数は `std::array` / 値メンバ（`WorldTransform worldTransform_`, `AttackEffect attackEffect_`）、可変数・多態は `std::vector<T*>` — 根拠: DeathParticles.h:54, Player.h:117, GameScene.h:93-100
- **メンバ初期化** — クラス内初期化子で既定値 → 実行時状態は `Initialize()` で設定。コンストラクタ初期化子リストは 0 件、コンストラクタは空かデストラクタ対の形だけ — 根拠: Player.h:16-20, Player.cpp:93-107, GameScene.cpp:40, TitleScene.cpp:7
- **ポインタ渡し vs 参照渡し** — 保持する依存（Model/Camera/MapChipField/StageManager）はポインタで受け取ってメンバに保存、入力データは `const T&`、出力は `T&`、読み取り専用の依存は `const Player*` — 根拠: BaseEnemy.h:24/50, MapChipField.h:93, Player.h:160, CameraController.h:79
- **エンジン API への参照渡し** — `model_->Draw(worldTransform_, *camera_)` とポインタを逆参照して渡す — 根拠: Player.cpp:527
- **nullptr チェック** — 必須依存は `assert(model)` / `assert(mapChipField_)`、任意のもの（`deathParticles_`, 空白セルの `WorldTransform*`）は `if (ptr)` で分岐、`delete` 後に `nullptr` を代入 — 根拠: Player.cpp:95/539, GameScene.cpp:350/365/418, main.cpp:35-36
- **assert の使い方** — 前提条件（NULL・範囲・ファイル存在）に使用、`assert(cond && "日本語メッセージ")` 慣用句、到達禁止は `assert(false && "…")`、I/O 失敗は MessageBoxA → `assert(0)` → return — 根拠: StageManager.h:27, StageManager.cpp:15/27, GlobalVariables.cpp:178-183
- **時間の扱い** — フレーム基準。秒単位のタイマーに毎フレーム `1.0f / 60.0f` を加算、長さは秒の定数（`kDuration = 0.3f`）、速度は「1 フレームあたり」の量、コヨーテタイムだけ int フレーム数 — 根拠: Fade.cpp:31, AttackEffect.h:46, Player.h:94-95, Player.h:135
- **状態管理** — `enum class` + `switch`（関数ポインタ/テーブルは不使用）。状態ごとに `UpdateRoot / UpdateAttack / UpdateKnockback / UpdateDead` に分割し、切替は `std::optional` の要求 → 次フレーム頭で `BehaviorAttackInitialize` — 根拠: Player.cpp:109-143, Player.h:175-187, BaseEnemy.cpp:48-59
- **更新処理の分割** — 大きな Update は `#pragma region` と小関数（`MoveWithMapCollision`, `UpdateOnGround`, `RestrictToScreen`, `CheckMapCollisionRight/Left/Vertical`, `FindFloorSurfaceY`）に分ける — 根拠: Player.h:160-195, Player.cpp:156-285
- **早期 return** — 終了済み・該当方向でない・範囲外は先頭で return — 根拠: AttackEffect.cpp:43-45, Player.cpp:750-752, MapChipField.cpp:94-99
- **変換テーブルは std::map / 配列で持つ** — 文字列→enum、向き→角度、角→オフセット — 根拠: MapChipField.cpp:15-36, Player.cpp:337, Player.cpp:589-594
- **デバッグ機能の分離** — `#ifdef _DEBUG`（キー操作・開始設定）と `#ifdef USE_IMGUI`（ImGui UI）を明確に分けて囲む — 根拠: GameScene.cpp:301-338, main.cpp:85/157/182
- **「調整項目」パターン** — 全てのチューニング値をクラスの `static inline` に置き、`RegisterGlobalVariables`（既定値登録）/ `ApplyGlobalVariables`（反映）の 2 関数を各クラスに同じ形で実装 — 根拠: Player.cpp:13-86, Enemy.cpp:9-46, CameraController.cpp:11-43
- **数値型の選択** — 添字・個数は `uint32_t`、ステージ番号は `int32_t`、カウンタは `int`、ループは `size_t` と `uint32_t` が混在、浮動小数は全て `float` + `f` 接尾辞（527 箇所、`.f` 省略形 0） — 根拠: MapChipField.h:111, StageManager.h:56, GameScene.cpp:318, StageManager.cpp:7, GameScene.cpp:462、grep 集計

### 書式

- **インデント** — タブ（3,634 行がタブ始まり、スペース始まりは 16 行で全て初期化子リストの継続行） — 根拠: grep 集計、MapChipField.cpp:16-28
- **改行コード** — リポジトリ内は LF、作業ツリーは CRLF（`* text=auto`） — 根拠: `git ls-files --eol` = `i/lf w/crlf` 40 件、.gitattributes:1
- **文字コード** — UTF-8（.editorconfig で `charset = utf-8`）、日本語コメント・日本語 `#pragma region` 名・日本語 assert メッセージを含む — 根拠: .editorconfig:1-2, Player.cpp:148, StageManager.cpp:15
- **波括弧** — K&R（関数・制御文とも開き括弧は同じ行、`} else {`）。`if` の単文でも原則 `{}` を付ける。例外は `continue` の 2 箇所（次行に書く） — 根拠: Player.cpp:207, Player.cpp:101-102, GameScene.cpp:418-419, GlobalVariables.cpp:288-289
- **`else if` の前にコメントを挟む形** — `}` / `// 説明` / `else if (…) {` と else を行頭に置く書き方が GlobalVariables.cpp にある — 根拠: GlobalVariables.cpp:150-157, 255-262, 304-311
- **1 行の長さ** — 上限なしに近い。最長 180 文字、120 文字超が 179 行 / 5,206 行（長い式・長いコメントを折り返さない） — 根拠: CameraController.cpp:81, Player.cpp:420/644、集計
- **空行とブロック構造** — 「`// 説明` 1 行 → 数行のコード → 空行」の繰り返し。関数間は 1 空行、`#pragma region` 前後にも空行 — 根拠: Player.cpp:164-210, GameScene.cpp:42-111
- **コメント量** — コメント専用行 1,308 行（約 25%）、うち `///` 358 行。ほぼ全ブロックに日本語の説明があり、理由・不具合対策の経緯を複数行で書く — 根拠: 集計、Player.cpp:637-642/692-698
- **行末コメントの位置揃え** — 連続する宣言・呼び出しの行末コメントをスペースで揃える — 根拠: GameScene.cpp:55-65, Player.cpp:590-593
- **`///` と `//` の使い分け** — ヘッダの型・public 関数に `/// <summary>`、それ以外は `//`。`#pragma region` の見出しにも日本語 — 根拠: Fade.h:4-18, GameScene.cpp:240
- **マジックナンバー** — 原則 `k` 定数へ抽出（Player.h:22-140）。残っている直書き: `0.8f`（ジャンプ上昇カット）、`1.0f / 60.0f`（多数）、`1280, 720`、`{100, 50}`、`0.01f`（Drag 速度）、`3.14159265358979323846f`（GameScene.cpp のみ。他は `std::numbers::pi_v<float>`） — 根拠: Player.cpp:252, Fade.cpp:31, GameScene.cpp:70/51/12, GlobalVariables.cpp:307, TitleScene.cpp:28
- **`this->`** — 0 件 — 根拠: grep 集計
- **整形ツールの痕跡** — `.clang-format` は無いが、include のアルファベット順・初期化子リストの 4 スペース継続インデント・長い 1 行引数など機械整形らしい形 — 根拠: Player.h:2-6, MapChipField.cpp:15-29, CameraController.cpp:81

### 気になる点（一貫していない箇所・2 通りの書き方）

- **クラス内の public/private の順** — 旧（GameScene/Player/Skydome）は private 先、新（BaseEnemy/Fade/HitEffect/CameraController/StageManager 等）は public 先 — 根拠: GameScene.h:43/119, BaseEnemy.h:12/82
- **終了フラグの名前** — `finished_`（TitleScene/GameScene）と `isFinished_`（Fade 以外のエフェクト・BaseEffect・DeathParticles） — 根拠: TitleScene.h:60, GameScene.h:115, BaseEffect.h:47
- **状態関数の命名** — `UpdateRoot / UpdateAttack`（動詞先）と `BehaviorAttackInitialize`（名詞先）が同じクラスに同居 — 根拠: Player.h:175-187
- **円周率** — `std::numbers::pi_v<float>` と `const float pi = 3.14159265358979323846f` の併用 — 根拠: TitleScene.cpp:28, GameScene.cpp:12
- **エフェクトの構造** — BaseEffect 継承の使い捨て型（HitEffect/GuardEffect）と、非継承の再利用型（AttackEffect）が混在。共有モデルの渡し方も `StaticInitialize` と `Initialize(model, camera)` の 2 通り — 根拠: HitEffect.h:23, AttackEffect.h:15
- **モデルの所有場所** — GameScene が生成して渡す（Player/Enemy）方式と、クラス自身が `CreateFromOBJ` する（Skydome / TitleScene）方式 — 根拠: GameScene.cpp:55-65/189, Skydome.cpp:7, TitleScene.cpp:20-21
- **ループ添字の型** — `size_t`（StageManager.cpp:7, GameScene.cpp:572）と `uint32_t`（GameScene.cpp:462）と `int`（Matrix4x4.cpp:10, Player.cpp:620）が混在
- **include 順** — GameScene.h だけ非アルファベット順（依存順）、他はアルファベット順 — 根拠: GameScene.h:2-15, Player.h:2-6
- **`default:` の置き方** — `case kRoot: default:` とまとめる（Player.cpp:130-131, BaseEnemy.cpp:51-52）／`default: break;` 単独（main.cpp:52-53, GameScene.cpp:490-491）／default なし（Fade.cpp:25-47, GameScene.cpp:186-215）
- **ドキュメントコメントの付け方** — GameScene.h の public 関数は `//`、他クラスは `///`；Player.h:202 に `/// /// <param` のタイプミス — 根拠: GameScene.h:123-148, Player.h:198-204
- **enum class の整数値コメント** — MapChipType は列挙子ごとに `// 0 空白` と数値を併記するが CSV は文字コード（B0 等）で対応付けており数値は使われない — 根拠: MapChipField.h:8-25, MapChipField.cpp:15-29
- **メンバ `camera_ = camera;` の二重代入** — Player::Initialize で 2 回代入 — 根拠: Player.cpp:99, 106
- **`#include <array>` の未使用 include** — Player.cpp は std::array を使っていない — 根拠: Player.cpp:7
- **長い 1 行 vs 分割** — `std::clamp` の長い引数を 1 行に書く箇所（CameraController.cpp:81-82）と、一時変数に分けて書く箇所（Player.cpp:490-495）が混在
