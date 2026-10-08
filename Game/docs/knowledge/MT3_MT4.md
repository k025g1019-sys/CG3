# MT3 / MT4 知識項目インベントリ（MT4 リポジトリ）

## 0. 調査範囲と前提
- 対象: `C:\Users\s-dai\source\repos\MT4` の HEAD（22453e1 = tag MT4_00_01）にある全 *.h / *.cpp 45 ファイル（計 3,731 行）を全文読了。加えてタグ MT3_00_01〜MT4_00_01 の全コミットを `git log --stat` / `git diff --stat` / `git show <tag>:<file>` で確認し、HEAD で消えた機能を補った。
- 旧スナップショット `repos\MT3_00_01`〜`MT3_01_02`（各 main.cpp 1 本、git 管理なし）は対応タグ（MT3_01_02 はコミット ab28b04）と内容同一（改行位置・BOM・ウィンドウタイトル "GC1A_04_…" の差のみ）。追加の知識項目は無し。
- ビルド設定: C++20（Novice.vcxproj `<LanguageStandard>stdcpp20`）、Novice / ImGui は KamataEngine 側（`C:\KamataEngine\...`）の include。ImGui 関連コードは `#ifdef _DEBUG` 内のみ。
- 行列規約: 行優先・行ベクトル（v × M）、平行移動は m[3][0..2]、DirectX 左手系・深度 0..1、ビューポート行列で Y 反転。
- 全履歴で一度も呼ばれていない関数群（FrustumDebug / Clipping / ToSpherical など）は E 節に明記した。

---

## A. C++ 言語機能・標準ライブラリ

- **struct（集成体）** — Vector3 / Vector4 / Matrix4x4 / Spherical / Ball / FrustumPlane / Frustum を public メンバだけの struct で定義し `{x, y, z}` の波括弧初期化で生成 — 根拠: Vector3.h:3-4, Vector4.h:2-4, Matrix4x4.h:4-6, Spherical.h:8-12, Spring.h:9-16, FrustumDebug.h:9-15
- **class と public / private** — 図形・物理・シーンを class 化し、データは private、操作は public — 根拠: Sphere.h:15-69, AABB.h:14-67, Scene.h:15-75
- **コンストラクタ（メンバ初期化子リスト・デフォルト引数）** — `Sphere(const Vector3& center, float radius, unsigned int color = 0xFFFFFFFF, bool enableGravity = false, float mass = 1.0f, float restitution = 0.8f)` を初期化子リストで初期化し、本体で条件付き初期化 — 根拠: Sphere.h:18-23, AABB.h:17-18, OBB.h:20-26, Segment.h:16-17, Curve.h:16-17, Triangle.h:15-19
- **`= default` コンストラクタ / デストラクタ** — `Sphere() = default;`、`virtual ~Scene() = default;` — 根拠: Sphere.h:17, AABB.h:16, OBB.h:18, Plane.h:16, Segment.h:15, Curve.h:15, Scene.h:17
- **コンストラクタ本体での初期化** — 物理クラスは本体代入で初期値を設定（`anchor_ = {...}; stiffness_ = 100.0f;`） — 根拠: Spring.cpp:11-25, CircularMotion.cpp:13-24, Pendulum.cpp:12-27, ConicalPendulum.cpp:12-28
- **デフォルトメンバ初期化子（NSDMI）** — `Vector3 position{0.0f, 1.9f, -6.49f};`、`float deltaTime_ = 1.0f / 60.0f;`、`bool isHit_ = false;`、クラス内で `MakeIdentity4x4()` 呼び出し初期化 — 根拠: Camera.h:20-24, Sphere.h:47-68, OBB.h:59-73, Scene.h:67-71
- **継承・仮想関数・純粋仮想・override** — 抽象クラス Scene（`virtual void Update(SceneManager&) = 0; virtual void Draw() = 0;`）を TitleScene / GameScene が public 継承し `override` — 根拠: Scene.h:15-20, 37-41, 57-61
- **仮想デストラクタ** — 基底 Scene に `virtual ~Scene() = default;`（unique_ptr<Scene> 経由の破棄のため） — 根拠: Scene.h:17
- **演算子オーバーロード（メンバ）** — Vector3 の `+=`, `-=`, `*=`(float), `/=`(float)、`operator[]`（非 const / const の 2 版、`*(&x + i)` で x,y,z を添字参照） — 根拠: Vector3.h:7-39
- **演算子オーバーロード（非メンバ inline）** — Vector3 の `+`, `-`, `*`（vec×scalar と scalar×vec の両順）, `/`(scalar), 単項 `-`, 単項 `+` — 根拠: Vector3.h:43-59
- **演算子オーバーロード（行列）** — Matrix4x4 の `+`, `-`, `*` を非メンバで宣言し Add / Subtract / Multiply に委譲 — 根拠: Matrix4x4.h:8-10, Matrix4x4.cpp:6-8
- **演算子の実利用例** — `rotateXMatrix * rotateYMatrix * rotateZMatrix`、`velocity_ += acceleration_ * deltaTime_`、`-stiffness_ * displacement`、`force / ball_.mass`、`input - 2.0f * Dot(input, normal) * normal` — 根拠: Scene.cpp:38-48, Sphere.cpp:20-21, Spring.cpp:40-50, Matrix4x4.cpp:318
- **演算子オーバーロードの変遷（履歴）** — MT3_02_04 でメンバ `operator+`、MT3_02_05 で `operator-`、8bed0be で `operator*`/`[]`、MT3_03_02 で現在の一式へ — 根拠: git log -p Vector3.h（3c1c07a, 49d773c, 8bed0be, 761b3d5）
- **関数オーバーロード** — 同名 Add / Subtract / Multiply をベクトル用・行列用で、Multiply(v,s) / Multiply(s,v)、Length(v) / Length(p1,p2) を引数で区別 — 根拠: Matrix4x4.h:15-19, 73-79, 91-94, Collision.h:12
- **inline 関数** — Vector3 の自由演算子をヘッダ内 `inline` 定義 — 根拠: Vector3.h:43-59
- **テンプレート関数** — `template<class T> void DrawObjectTree(const char*, std::vector<T>&, const char*)` で全図形の ImGui ツリーを共通化 — 根拠: Objects.cpp:317-334
- **ラムダ式（[&] キャプチャ）** — `auto drawLine = [&](int i, int j) {...}` で箱の 12 辺描画を共通化、`auto DrawEdge = [&](int a, int b)` — 根拠: AABB.cpp:44, OBB.cpp:110, FrustumDebug.cpp:108
- **ジェネリックラムダ（auto 引数）** — `auto drawList = [&](auto& container) { for (auto& obj : container) obj.Draw(...); }` を 12 種の vector に適用 — 根拠: Objects.cpp:288-305
- **auto / const auto&** — `const auto& axis = obb.GetOrientations();`、`const auto& p = frustum.planes[i];` — 根拠: Collision.cpp:241, 277, 317-318, FrustumDebug.cpp:66
- **範囲 for** — `for (auto& sphere : spheres)` でコンテナ走査 — 根拠: Objects.cpp:103-132, 266-280, Objects.cpp:289
- **添字 for（ペア総当たり）** — `for (size_t i...) for (size_t j = i + 1; ...)` で同種同士を重複なしに総当たり — 根拠: Objects.cpp:139-146, 186-193, 243-250
- **三項演算子** — `(i == j) ? 1.0f : 0.0f`、`keys[DIK_D] ? moveSpeed : -moveSpeed`、`isHit ? kHitColor : kNormalColor` — 根拠: Matrix4x4.cpp:109, Camera.cpp:13, Objects.cpp:267
- **const 修飾（const メンバ関数・const 参照引数・const 戻り値）** — `const Vector3& GetMin() const`、`const Vector3 GetCenter() const`、関数引数は `const Matrix4x4&` — 根拠: AABB.h:28-36, Sphere.h:33-36, Matrix4x4.h:15-97
- **constexpr / const 定数** — `constexpr uint32_t kHitColor = 0xFF0000FF;`、`constexpr float EPSILON = 1e-6f;`、`const int kWindowWidth = 1280;`、関数内 `const uint32_t kSubdivision = 12;` — 根拠: Objects.cpp:94-95, Collision.cpp:323, Scene.h:9-10, Sphere.cpp:52-54
- **static（内部リンケージ関数・static const 変数）** — `static Vector3 NormalizeVector(...)`、`static Vector4 Lerp(...)`、`static bool ClipPlane(...)`、`static const int kRowHeight = 20;` — 根拠: OBB.cpp:12, Clipping.cpp:4, 28, DebugGrid.cpp:72-73
- **無名名前空間** — 色定数とテンプレートヘルパを `namespace { ... }` に隔離 — 根拠: Objects.cpp:92-97, 315-336
- **前方宣言** — `struct Matrix4x4; class Sphere; ...` でヘッダ依存を削減 — 根拠: Objects.h:4-16, Collision.h:3-9, Camera.h:4, Spherical.h:3
- **参照・ポインタ** — 参照渡しの出力引数 `Matrix4x4& viewProjectionMatrix`、`const char* keys`、配列先頭ポインタを返す `const Vector3* GetVertices()`、raw ポインタメンバ `Objects *objects` — 根拠: Camera.h:8, Triangle.h:27, Scene.h:65
- **ポインタ演算によるメンバ添字アクセス** — `(&p0.x)[i]`、`*(&x + i)` で x/y/z を配列的に扱う — 根拠: Collision.cpp:194-198, Vector3.h:38-39
- **new（raw ポインタ、delete なし）** — `objects = new Objects;`（解放処理なし） — 根拠: Scene.cpp:84, Scene.h:65
- **new / delete による手動管理（履歴）** — MT3_02_02〜03 の SceneManager は `Scene* current/next` を `delete current;` で切替・解放 — 根拠: git show MT3_02_03:Scene.cpp 12-21, MT3_02_03:Scene.h 26-31
- **std::unique_ptr / std::make_unique / std::move** — SceneManager が `std::unique_ptr<Scene> current, next;`、`SetScene(std::make_unique<GameScene>())`、`current = std::move(next);`（MT3_02_04 で導入） — 根拠: Scene.h:6, 25-30, Scene.cpp:13-17, main.cpp:13
- **std::vector** — 図形種ごとの `std::vector<Sphere>` 等 12 本、`spheres = { Sphere(...), };` と初期化子リストで代入、`.size()`、`[i]`、`.empty()`（履歴） — 根拠: Objects.h:2, 28-39, Objects.cpp:21-88, 139; git show MT3_02_03:Scene.cpp 148
- **std::array** — `std::array<Vector3, 3>` を OBB 軸・ベジエ制御点・階層の SRT に使用、`{{{...}}}` 二重波括弧初期化 — 根拠: OBB.h:2, 20, 63-67, Curve.h:16, 36-40, Hierarchy.h:15-31
- **C 配列（固定長）** — `float m[4][4]`、`Vector3 vertices_[3]`、`Matrix4x4 localMatrices_[3]`、`Vector3 v[8]`、`char keys[256]`、`float R[3][3]` — 根拠: Matrix4x4.h:5, Triangle.h:36, Hierarchy.h:33-34, AABB.cpp:25-38, Scene.h:44-45, Collision.cpp:332-333
- **16 個の float による行列の集成体初期化（履歴）** — `Matrix4x4 m1 = {3.2f, 0.7f, ...};` — 根拠: git show MT3_00_02:main.cpp 45-47
- **std::string / std::to_string（履歴）** — ImGui ラベルを `"Sphere[" + std::to_string(i) + "].center"` で動的生成（MT3_02_01〜02 のみ、後に PushID 方式へ） — 根拠: git show MT3_02_01:main.cpp 232-237, 0a84dba:Scene.cpp 79-92
- **<algorithm> std::clamp** — AABB 最近接点、線分の t、仰角制限、asin 引数の丸め — 根拠: Collision.cpp:164-166, Segment.cpp:22, Scene.cpp:157, Spherical.cpp:18
- **<algorithm> std::min / std::max** — スラブ法の tNear / tFar、AABB の min/max 正規化、`(std::max)(...)` と括弧で Windows.h の max マクロ衝突を回避 — 根拠: Collision.cpp:67, 211-217, AABB.cpp:9-10, Scene.cpp:155-156
- **std::swap** — 逆行列のピボット行交換で `std::swap(a.m[i], a.m[r])`（float[4] 配列同士） — 根拠: Matrix4x4.cpp:57-59
- **<cmath>** — `std::sqrt`, `std::sin`, `std::cos`, `std::tan`, `std::fabs`, `std::abs`, `std::atan2`, `std::asin`、C 版 `sqrtf`, `cosf`, `sinf`, `fabs` — 根拠: Matrix4x4.cpp:53, 152-153, 238, 331, Sphere.cpp:64-66, OBB.cpp:14, Spherical.cpp:14-23, Collision.cpp:200
- **M_PI（_USE_MATH_DEFINES）** — `#define _USE_MATH_DEFINES` を <cmath> より前に置いて `M_PI` を使用 — 根拠: Sphere.cpp:1, 53-57, CircularMotion.cpp:1, 17
- **<numbers> std::numbers::pi_v<float>（C++20）** — 球面座標の初期値と仰角クランプに使用 — 根拠: Scene.h:7, 71, Scene.cpp:154
- **<assert.h> assert** — `assert(w != 0.0f);` で同次座標 w の 0 除算を検出 — 根拠: Matrix4x4.cpp:3, 142
- **memcpy** — `memcpy(preKeys, keys, 256);` で前フレームのキー状態を保存（<cstring> は Novice.h 経由） — 根拠: Scene.cpp:53, 94
- **固定幅整数・size_t・書式** — `uint32_t`, `int32_t`, `size_t`、printf 書式 `%zu` / `%8.3f` / `%6.02f` — 根拠: DebugGrid.cpp:13, 84, Plane.cpp:25, Objects.cpp:139, 325, Scene.cpp:172
- **キャスト** — `static_cast<int>` / `static_cast<float>`、関数形式 `int(x)` / `float(i)`、C 形式 `(int)sv[i].x` — 根拠: DebugGrid.cpp:21, 34, Curve.cpp:28, 46, AABB.cpp:44
- **波括弧初期化の戻り値・二重波括弧** — `return {v1.x + v2.x, ...};`、Matrix4x4 を `return {{ {right.x, ...}, {up...}, {forward...}, {eye.x, ..., 1.0f} }};` — 根拠: Matrix4x4.cpp:298-304, 222-227
- **指示付き初期化（C++20、履歴）** — `Sphere({.center = {...}, .radius = 0.5f, .moveSpeed = 0.03f})`、`Plane({.normal = ..., .distance = 1.5f})` — 根拠: git show 0a84dba:Scene.cpp 54-61, MT3_02_03:Scene.cpp 55-66
- **Desc 構造体によるコンストラクタ引数（履歴）** — SphereDesc / PlaneDesc / SegmentDesc をデフォルト値付きで定義し class に渡す（MT3_02_02〜03、MT3_02_04 で通常引数に変更） — 根拠: git show MT3_02_03:Structure.h 8-13, 47-50, 79-83
- **bool 型フラグとその判定関数** — `isHit_` / `IsHit()`、`enableGravity_` / `IsGravityEnabled()`、`isStart_`、`isDrawControlPoints_` — 根拠: Sphere.h:36, 43-44, 49, 53, 68, Curve.h:42
- **Getter / Setter** — `GetCenter / SetCenter`、`GetNormal / SetNormal`（正規化付き）、`SetMin / SetMax / SetMinMax`（正規化付き）、`SetOrientation(index, axis)`（範囲チェック付き） — 根拠: Sphere.h:33-41, Plane.h:30-34, AABB.h:46-58, OBB.h:47-51
- **double と float の混在** — `double distance = Length(...)` を float と比較 — 根拠: Collision.cpp:23-24
- **プリプロセッサ** — `#pragma once`、`#pragma region / endregion`、`#ifdef _DEBUG ... #endif`（ImGui コードの切替）、`#include <...>`（外部）と `"..."`（自作）の使い分け — 根拠: Vector3.h:1, Matrix4x4.h:12-27, Objects.h:23-25, Scene.cpp:1-7
- **`#ifdef ImGui` ガード（履歴の誤り）** — MT3_02_00 では未定義マクロ `ImGui` で囲んだため ImGui ブロックはコンパイルされず、MT3_02_04 で `_DEBUG` に統一 — 根拠: git show MT3_02_00:main.cpp 8-10, 208-211; git show 3c1c07a -- Camera.cpp
- **Windows エントリポイント** — `int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)` — 根拠: main.cpp:7
- **グローバル定数** — `const char kWindowTitle[] = "Window";`、ヘッダ内 `const int kWindowWidth/kWindowHeight` — 根拠: main.cpp:4, Scene.h:9-10
- **コメント規約** — `/// <summary>` XML コメント、`//!<` Doxygen 風メンバコメント、#pragma region による区分け — 根拠: AABB.h:8-13, 63-64, Sphere.h:9-14

---

## B. 数学・物理・当たり判定

### B-1 ベクトル（Vector3）
- **加算 Add / 減算 Subtract** — 成分ごとの加減（MT3_00_01 が初出） — 根拠: Matrix4x4.cpp:298-301; git show MT3_00_01:main.cpp 13-16
- **スカラー倍 Multiply(v, s) / Multiply(s, v)** — 成分×スカラー（MT3_00_01 では `Multipry` と綴り誤り） — 根拠: Matrix4x4.cpp:304, 325-327
- **内積 Dot** — `x1x2 + y1y2 + z1z2` — 根拠: Matrix4x4.cpp:295
- **外積 Cross** — `(y1z2−z1y2, z1x2−x1z2, x1y2−y1x2)`（MT3_01_01 で追加） — 根拠: Matrix4x4.cpp:282-288
- **長さ Length(v)** — `sqrt(x²+y²+z²)` — 根拠: Matrix4x4.cpp:330-332
- **2 点間距離 Length(p1, p2)** — 差の二乗和の平方根 — 根拠: Collision.cpp:14-16
- **正規化 Normalize** — 長さで割る、長さ < 1e-6 なら零ベクトルを返す（0 除算防止） — 根拠: Matrix4x4.cpp:335-344（別実装 OBB.cpp:12-21 NormalizeVector は `== 0.0f` 判定）
- **正射影ベクトル Project(v1, v2)** — `t = (v1·v2)/(v2·v2)` で `v2·t`、v2 が零ベクトルなら零 — 根拠: Matrix4x4.cpp:307-314
- **反射ベクトル Reflect** — `r = v − 2(v·n)n`（n は単位法線） — 根拠: Matrix4x4.cpp:317-319
- **最近接点 ClosestPoint(点, 線分)** — `t = ((p−o)·d)/(d·d)` を [0,1] にクランプし `o + d·t`、d が零なら origin — 根拠: Segment.cpp:12-25（MT3_02_00 では自由関数: git show MT3_02_00:Matrix4x4.cpp 293-307）
- **垂直ベクトル Perpendicular** — `|x| > |y|` なら `(−y, x, 0)`、それ以外は `(0, −z, y)` — 根拠: Plane.cpp:8-13
- **線形補間 Lerp** — `v1 + (v2 − v1)·t`（Vector3 版は Curve のメンバ、Vector4 版は Clipping の static 関数） — 根拠: Curve.cpp:13-20, Clipping.cpp:4-11
- **Gram–Schmidt 直交化** — OBB の軸を x 正規化 → y から x 成分を除去して正規化 → z = x×y で直交基底を再構成、半サイズは abs — 根拠: OBB.cpp:26-49
- **別基底への射影（ローカル座標化）** — `Dot(local, axis[i])` で OBB ローカル座標に変換 — 根拠: Collision.cpp:289-291, 329

### B-2 行列（Matrix4x4）
- **行列の加算 Add / 減算 Subtract** — 成分ごとの二重ループ（MT3_00_02 が初出） — 根拠: Matrix4x4.cpp:13-32
- **行列の積 Multiply** — `result[i][j] = Σ_k m1[i][k]·m2[k][j]`（行優先） — 根拠: Matrix4x4.cpp:35-43
- **逆行列 Inverse** — ガウス・ジョルダン法（単位行列を並べて掃き出し、ピボット |a| < 1e-6 なら下の行と交換、失敗時は単位行列を返す） — 根拠: Matrix4x4.cpp:46-91
- **転置行列 Transpose** — `result[i][j] = m[j][i]`（HEAD では未使用、MT3_00_02 で表示のみ） — 根拠: Matrix4x4.cpp:94-102
- **単位行列 MakeIdentity4x4** — 対角 1 — 根拠: Matrix4x4.cpp:105-113
- **平行移動行列 MakeTranslateMatrix** — `m[3][0..2] = translate`（行ベクトル規約） — 根拠: Matrix4x4.cpp:120-126
- **拡大縮小行列 MakeScaleMatrix** — 対角に scale — 根拠: Matrix4x4.cpp:128-134
- **座標変換 Transform(v, M)** — `v × M`（w = 1）を計算し w で除算（透視除算）、`assert(w != 0)` — 根拠: Matrix4x4.cpp:136-147
- **回転行列 MakeRotateX / Y / ZMatrix** — cos / sin による各軸回転（行ベクトル用の符号配置: X は m[1][2]=s, m[2][1]=−s） — 根拠: Matrix4x4.cpp:150-189
- **回転の合成 X→Y→Z** — `R = Rx · (Ry · Rz)` — 根拠: Matrix4x4.cpp:201, OBB.cpp:51-58, Scene.cpp:48
- **アフィン変換行列 MakeAffineMatrix(S, R, T)** — `S · R · T` の順で合成（ワールド行列） — 根拠: Matrix4x4.cpp:192-207
- **透視投影行列 MakePerspectiveFovMatrix** — `yScale = 1/tan(fovY/2)`, `xScale = yScale/aspect`, `m[2][2] = f/(f−n)`, `m[2][3] = 1`, `m[3][2] = −nf/(f−n)`（左手系・深度 0..1） — 根拠: Matrix4x4.cpp:235-248
- **正射影行列 MakeOrthographicMatrix** — `2/(r−l)`, `2/(t−b)`, `1/(f−n)` と平行移動項（HEAD 未使用、MT3_01_00 で表示のみ） — 根拠: Matrix4x4.cpp:251-264
- **ビューポート行列 MakeViewportMatrix** — `w/2`, `−h/2`（Y 反転）, `maxDepth−minDepth`、平行移動 `left + w/2`, `top + h/2`, `minDepth` — 根拠: Matrix4x4.cpp:267-279
- **ビュー行列（逆変換の手組み）** — `Rx(−rx) · Ry(−ry) · Rz(−rz) · T(−pos)` を Multiply で合成 — 根拠: Camera.cpp:33-42
- **ビュー行列（逆行列方式、履歴）** — `cameraMatrix = MakeAffineMatrix(...)`, `viewMatrix = Inverse(cameraMatrix)`（MT3_01_01） — 根拠: git show MT3_01_01:main.cpp 130-132
- **注視点カメラ行列 MakeLookAtCameraMatrix(eye, target)** — forward = normalize(target−eye)、right = normalize(worldUp × forward)、up = forward × right を各行に、第 4 行に eye（ビュー行列はこの逆行列、とコメント） — 根拠: Matrix4x4.cpp:210-228, Matrix4x4.h:48-49
- **行列からの軸・位置の取り出し** — 回転行列の各行を OBB の軸に、ワールド行列の第 4 行 m[3][0..2] を関節位置に使用 — 根拠: OBB.cpp:62-68, Hierarchy.cpp:25-28
- **VP と Viewport の事前合成** — `vpvMatrix = Multiply(viewProjection, viewport)` で 1 回の Transform に短縮 — 根拠: DebugGrid.cpp:17, 28-29
- **視錐台 6 平面の抽出（Gribb–Hartmann）** — VP 行列の列の和差（col3 ± col0 → left/right、col3 ± col1 → bottom/top、col2 → near、col3 − col2 → far）で平面の法線と距離を得る — 根拠: FrustumDebug.cpp:8-48
- **平面の正規化 NormalizePlane** — 法線長で normal と distance を割る — 根拠: FrustumDebug.cpp:50-57
- **NDC → ワールドの逆変換** — `Inverse(VP)` で NDC 立方体の 8 頂点（z は 0..1）をワールドへ戻して視錐台を線描 — 根拠: FrustumDebug.cpp:82-105
- **同次クリップ座標 TransformToClip** — w 除算せず (x, y, z, w) を返す — 根拠: Clipping.cpp:13-26
- **線分クリッピング（6 平面・パラメトリック）** — 各平面の距離 `x+w, −x+w, y+w, −y+w, z, −z+w` の符号で内外判定し `t = da/(da−db)` で交点補間、両端とも外なら棄却 — 根拠: Clipping.cpp:28-87
- **クリップ → NDC → スクリーン** — `clip/w` → `Transform(ndc, viewport)` → DrawLine — 根拠: Clipping.cpp:89-120

### B-3 座標変換・レンダリングパイプライン
- **ローカル → ワールド → ビュー → 射影 → NDC → スクリーン** — `WVP = world · (view · projection)`、`Transform(local, WVP)` → `Transform(ndc, viewport)`（MT3_01_01 の三角形） — 根拠: git show MT3_01_01:main.cpp 130-139, 153-154
- **ワールド → スクリーン（2 段 Transform）** — `Transform(Transform(p, viewProjection), viewport)` を全 Draw で使用 — 根拠: Sphere.cpp:68-75, Segment.cpp:33-34, Curve.cpp:42-44, Spring.cpp:55-56, Triangle.cpp:14-20
- **階層構造（親子行列）** — `local[i] = MakeAffineMatrix(S,R,T)`、`world[0] = local[0]`、`world[i] = local[i] · world[i−1]`（肩→肘→手） — 根拠: Hierarchy.cpp:8-19
- **ワールド空間 AABB** — `min + position`, `max + position` でローカル AABB を位置オフセット — 根拠: AABB.h:38-44
- **OBB の 8 頂点生成** — `center ± xAxis·hx ± yAxis·hy ± zAxis·hz` — 根拠: OBB.cpp:78-96
- **球面座標 → 直交座標 ToCartesian** — `rho = r cos θ`、`(rho cos φ, r sin θ, rho sin φ)`（θ 仰角、φ 方位角、+X→+Z が正） — 根拠: Spherical.cpp:6-11, Spherical.h:8-12
- **直交座標 → 球面座標 ToSpherical** — `r = |p|`, `θ = asin(clamp(y/r, −1, 1))`, `φ = atan2(z, x)`（未呼び出し） — 根拠: Spherical.cpp:13-24
- **球面座標カメラの特異点回避** — 半径 ≥ 0.1、|θ| ≤ π/2 − 0.01 にクランプしてから `eye = target + ToCartesian(spherical)` — 根拠: Scene.cpp:153-163

### B-4 補間・曲線
- **2 次ベジエ曲線（De Casteljau）** — `p01 = Lerp(c0,c1,t)`, `p12 = Lerp(c1,c2,t)`, `b = Lerp(p01,p12,t)` を 100 分割して線分で描画 — 根拠: Curve.cpp:22-48
- **制御点の可視化** — 半径 0.01 の黒い球で 3 制御点を描画、Checkbox で表示切替 — 根拠: Curve.cpp:50-63, 71
- **（備考）Slerp / Catmull-Rom / イージング関数は未実装** — 根拠: 全履歴 grep に該当なし

### B-5 物理
- **固定 Δt（1/60 秒）とオイラー積分** — `v += a·dt; p += v·dt;` — 根拠: Sphere.h:65, Sphere.cpp:19-21, Spring.cpp:47-50
- **重力加速度** — `acceleration = {0, −9.8, 0}`、Start ボタンで有効化 — 根拠: Sphere.h:20-22, Sphere.cpp:93-96
- **1 フレーム前位置の保存（スイープ判定用）** — `prevCenter_ = center_` — 根拠: Sphere.cpp:11-12, Sphere.h:54
- **平面での反発（反発係数 e）** — 1 フレーム前の位置で平面のどちら側かを決め、めり込み分を法線方向に押し戻し、平面へ向かう速度のときだけ Reflect、法線成分（Project）に e を掛け接線成分は保存 — 根拠: Sphere.cpp:24-48
- **ばね（フックの法則 + 減衰）** — `F = −k·(p − rest) − c·v`, `a = F/m`、rest = anchor + dir·naturalLength — 根拠: Spring.cpp:27-51, Spring.h:33-39
- **質量・加速度・速度を持つ Ball 構造体** — position / velocity / acceleration / mass / radius / color — 根拠: Spring.h:9-16
- **等速円運動** — `angle += ω·dt`、`p = center + (cos θ·r, sin θ·r, 0)`（XY 平面）、ω = π rad/s（2 秒で 1 周） — 根拠: CircularMotion.cpp:13-40
- **単振り子** — 角加速度 `α = −(g/L)·sin θ`、`ω += α·dt`, `θ += ω·dt`、先端 = anchor + (sin θ·L, −cos θ·L, 0) — 根拠: Pendulum.cpp:29-45
- **円錐振り子** — `ω = sqrt(g/(L·cos φ))`（φ = 半頂角）、半径 = L sin φ、高さ = L cos φ、位置 = anchor + (cos θ·r, −h, −sin θ·r) — 根拠: ConicalPendulum.cpp:30-46
- **Start / Reset によるシミュレーション制御** — `isStart_` が false の間は Update を早期 return、Reset で初期位置・速度に戻す — 根拠: Spring.cpp:28-31, Sphere.cpp:14-17, 98-103

### B-6 当たり判定（全組み合わせ）
- **球 – 球** — 中心間距離 ≤ 半径の和 — 根拠: Collision.cpp:21-25（初出: git show MT3_02_01:main.cpp 390-394 IsCollision）
- **球 – 平面** — 平面上の点 `d·n` から中心へのベクトルと法線の内積の絶対値（点と平面の距離）≤ 半径 — 根拠: Collision.cpp:30-48
- **カプセル（スイープ球）– 平面** — 両端点の符号付き距離 `p·n − d` の積が ≤ 0 なら貫通、そうでなければ近い方の距離 ≤ 半径（トンネリング防止、重力球専用） — 根拠: Collision.cpp:53-68, Objects.cpp:152-157
- **線分 – 平面** — `t = (d − o·n)/(n·dir)`、`n·dir == 0` は平行で不衝突、0 ≤ t ≤ 1 で衝突 — 根拠: Collision.cpp:73-92
- **線分 – 三角形** — 辺ベクトルの外積で法線、平面との t（|denom| < 1e-6 は平行）、交点 p、各辺×(p−頂点) の外積と法線の内積が全て ≥ 0 なら内側 — 根拠: Collision.cpp:97-144
- **AABB – AABB** — 3 軸それぞれで `a.min ≤ b.max && a.max ≥ b.min` — 根拠: Collision.cpp:149-154
- **AABB – 球** — 中心を各軸で clamp した最近接点との距離 ≤ 半径 — 根拠: Collision.cpp:159-172
- **AABB – 線分（スラブ法）** — 軸ごとに `t1 = (min−o)/d`, `t2 = (max−o)/d` → tNear/tFar、`tmin = max(tmin, tNear)`, `tmax = min(tmax, tFar)`、tmin > tmax で不衝突、平行軸（|d| < 1e-6）は始点の範囲チェック — 根拠: Collision.cpp:179-227
- **OBB – 球** — 中心差を OBB 各軸へ射影し半サイズで clamp → 最近接点、距離² ≤ r² — 根拠: Collision.cpp:233-266
- **OBB – 線分** — 線分を OBB ローカル空間へ変換（中心を引き各軸と内積）し、半サイズの AABB と線分で判定 — 根拠: Collision.cpp:273-306
- **OBB – OBB（分離軸定理 SAT・15 軸）** — A の 3 軸、B の 3 軸、外積 9 軸について射影半径 ra, rb と中心差の射影を比較、`R[i][j] = Dot(axisA[i], axisB[j])`、`AbsR = |R| + ε` — 根拠: Collision.cpp:312-433
- **OBB – AABB** — AABB を軸 (1,0,0),(0,1,0),(0,0,1) の OBB に変換（ConvertAABBToOBB）して OBB–OBB 判定へ委譲 — 根拠: Collision.cpp:439-459
- **球 – 視錐台（6 平面）** — 各平面の符号付き距離 `n·c + d < −r` なら完全に外（未呼び出し） — 根拠: FrustumDebug.cpp:59-79
- **判定結果の可視化** — ヒットフラグを集約し赤 (0xFF0000FF) / 白 (0xFFFFFFFF) で色分け — 根拠: Objects.cpp:94-95, 266-280
- **AABB の min/max 正規化** — ImGui 操作で min > max になっても成分ごとに入れ替えて修復 — 根拠: AABB.cpp:8-14

### B-7 デバッグ描画・カメラ
- **デバッググリッド** — XZ 平面に半幅 2・10 分割の線、x=0 の線を赤（Z 軸）、z=0 の線を青（X 軸）、原点から上へ緑の Y 軸 — 根拠: DebugGrid.cpp:11-66
- **ワイヤーフレーム球** — 緯度 −π/2..π/2・経度 0..2π を kSubdivision=12 で分割し、a–b（緯度方向）と a–c（経度方向）の線を引く — 根拠: Sphere.cpp:51-81
- **平面の四角形描画** — 中心 `d·n`、Perpendicular と Cross で 4 方向の直交ベクトルを作り 2 倍に延ばした 4 点を結ぶ — 根拠: Plane.cpp:17-34
- **線分描画** — 終点 = origin + diff をスクリーン変換して DrawLine — 根拠: Segment.cpp:27-38
- **三角形描画** — 3 頂点をスクリーン変換し Novice::DrawTriangle（ワイヤーフレーム） — 根拠: Triangle.cpp:10-31
- **箱（AABB / OBB）描画** — 8 頂点 → 12 辺（前面 4・背面 4・接続 4） — 根拠: AABB.cpp:20-63, OBB.cpp:76-135
- **行列・ベクトルの画面表示** — VectorScreenPrintf / MatrixScreenPrintf（ScreenPrintf を列幅 60・行高 20 で並べる） — 根拠: DebugGrid.cpp:72-87
- **フリーカメラ操作** — WASD で XY 移動、E/Q で Z 移動（2 倍速）、U/J・K/H・I/Y で X/Y/Z 回転、速度 0.04 / 0.01 — 根拠: Camera.cpp:11-31, Camera.h:20-24
- **カメラ初期値と射影パラメータ** — 位置 (0, 1.9, −6.49)、回転 (0.26, 0, 0)、fovY 0.45、near 0.1、far 100、ビューポート (0,0,1280,720, 0..1) — 根拠: Camera.h:20-21, Camera.cpp:46-48
- **ImGui によるカメラ・球面座標の数値編集** — DragFloat3 で位置/回転を Get→編集→Set、InputFloat で r/θ/φ、結果の行列を 4 行で表示 — 根拠: Scene.cpp:128-139, 149-151, 170-173

---

## C. ゲームの構造・実装パターン
- **ゲームループ** — `while (Novice::ProcessMessage() == 0) { BeginFrame(); manager.Update(); manager.Draw(); EndFrame(); }` — 根拠: main.cpp:20-47
- **Scene 抽象基底 + SceneManager** — `Update(SceneManager&)` / `Draw()` の純粋仮想、Manager が current / next を unique_ptr で保持し、`SetScene` は next に格納して次の Update 冒頭で切替（遅延切替） — 根拠: Scene.h:15-33, Scene.cpp:13-28
- **シーン遷移** — Title で Enter → GameScene、Game で Space → TitleScene、`manager.SetScene(std::make_unique<...>()); return;` — 根拠: Scene.cpp:68-70, 97-100
- **初期化 / 更新 / 描画の分離** — 各クラスが コンストラクタ（初期化）・`Update()`・`Draw(viewProjection, viewport)`・`DrawImGui()` を持つ — 根拠: Sphere.h:18-31, Spring.h:23-29, Hierarchy.h:7-12
- **オブジェクト集約クラス Objects** — 12 種の `std::vector` を保持し `UpdateAllCollisions()` → `Draw()` → `DrawImgui()` を一括実行、GameScene は Objects を 1 つ持つ — 根拠: Objects.h:18-40, Scene.cpp:84, 108, 120, 125
- **衝突フェーズの構造** — ①各オブジェクトの Update とヒットフラグのリセット → ②全組み合わせを判定しフラグ ON → ③最後に色を決定 — 根拠: Objects.cpp:102-132, 137-263, 265-280
- **同種ペアの重複排除** — `j = i + 1` から走査（旧版は `&a == &b` で自分自身をスキップ） — 根拠: Objects.cpp:139-140; git show 3c18c6f:Scene.cpp 113-115
- **コンテナ初期化による配置データ** — コンストラクタで `spheres = { Sphere(...), };`、他章の配置はコメントアウトで残す — 根拠: Objects.cpp:20-89
- **描画順序** — グリッド → オブジェクト → ImGui（Debug のみ） — 根拠: Scene.cpp:118-126
- **ImGui デバッグ UI の構成** — ウィンドウ "window" に TreeNode ごとの図形一覧、`PushID(i)` で同名ラベル衝突を回避、各クラスの DrawImGui に委譲 — 根拠: Objects.cpp:317-358, Sphere.cpp:85-106
- **ImGui 値の反映パターン** — メンバを直接 `&center_.x` で float[3] として渡す方式、Get→一時変数→Set 方式（Camera）、`if (ImGui::DragFloat3(...)) SetNormal(normal);` で変更時のみ正規化 — 根拠: AABB.cpp:68-71, Scene.cpp:130-137, Plane.cpp:39-42
- **Debug / Release 切替** — ImGui 関連を `#ifdef _DEBUG` で囲み Release から除外 — 根拠: Objects.h:23-25, Objects.cpp:309-362, Scene.cpp:122-178
- **入力処理（押下継続とトリガ）** — `GetHitKeyStateAll(keys)` + `memcpy(preKeys, keys, 256)`、トリガは `keys[k] && !preKeys[k]`、逆方向キーは `keys[A] != keys[B]` で排他 — 根拠: Scene.cpp:53-54, 68, 97, Camera.cpp:12-31
- **キー入力による球移動（履歴）** — 矢印キーで XZ、LSHIFT + 上下で Y 移動（`Sphere::UpdateToKeyMove`、MT3_02_01〜03） — 根拠: git show MT3_02_03:Structure.cpp 12-28
- **ヒット色の定数化** — `kHitColor` / `kNormalColor` を無名名前空間の constexpr に — 根拠: Objects.cpp:92-97
- **描画用の一時オブジェクト生成** — 関節・ボブ・制御点を描画時に `Sphere ball(position_, r, color); ball.Draw(...)` と一時生成 — 根拠: Hierarchy.cpp:32-47, Spring.cpp:62-63, Pendulum.cpp:56-57, Curve.cpp:52-53
- **窓サイズ定数の共有** — `kWindowWidth / kWindowHeight` を Scene.h で定義し main と Camera で使用 — 根拠: Scene.h:9-10, main.cpp:10, Scene.cpp:112
- **1 ファイルからの段階的分割** — MT3_02_00 で Vector3.h / Matrix4x4.*、MT3_02_02 で Scene / Camera / Structure / Collision、MT3_02_04 で図形ごとのファイル、MT3_02_07 で Objects へ — 根拠: git log --stat（a96671d, 0a84dba, 3c1c07a, 085c65f）
- **1 クラス 1 判定関数 → 統合（履歴）** — 085c65f〜8bed0be では `UpdateCollisionSphereSphere()` 等ペアごとのメンバ関数、MT3_02_08_EX でフラグ方式の `UpdateAllCollisions()` に統合 — 根拠: git show 8bed0be:Objects.h 15-23, 498bda4 の Objects.cpp 差分

---

## D. 使用しているフレームワーク / エンジン API
- **Novice::Initialize(title, width, height)** — ウィンドウ生成 1280×720 — 根拠: main.cpp:10
- **Novice::ProcessMessage()** — 0 以外で終了（×ボタン） — 根拠: main.cpp:20
- **Novice::BeginFrame() / EndFrame()** — フレーム開始・終了（描画確定） — 根拠: main.cpp:22, 41
- **Novice::Finalize()** — ライブラリ終了 — 根拠: main.cpp:50
- **Novice::GetHitKeyStateAll(char[256])** — 全キー状態の取得（DirectInput 配列） — 根拠: Scene.cpp:54, 95
- **Novice::DrawLine(x1, y1, x2, y2, color)** — 全ワイヤーフレーム描画の基本（球・グリッド・箱・線分・ばね） — 根拠: Sphere.cpp:77-78, DebugGrid.cpp:34, AABB.cpp:44, Spring.cpp:59
- **Novice::DrawTriangle(x1,y1,x2,y2,x3,y3,color,fillMode)** — `kFillModeWireFrame`（HEAD）/ `kFillModeSolid` + RED（MT3_01_01） — 根拠: Triangle.cpp:23-29; git show MT3_01_01:main.cpp 153-154
- **Novice::ScreenPrintf(x, y, fmt, ...)** — printf 書式のデバッグ文字列 — 根拠: DebugGrid.cpp:75-84
- **色の表現** — `0xRRGGBBAA` の unsigned int（0xFF0000FF 赤、0x00FF00FF 緑、0x0000FFFF 青、0xAAAAAAFF 灰、0xFFFF00FF 黄、0x000000FF 黒）と定数 WHITE / RED / BLUE — 根拠: DebugGrid.cpp:32, 47, 63, Objects.cpp:94-95, Curve.cpp:52, Spring.cpp:24, 59
- **DIK_ キーコード** — ESCAPE, RETURN, SPACE, W/A/S/D, E/Q, U/J, K/H, I/Y（HEAD）、UP/DOWN/LEFT/RIGHT, LSHIFT, O/L（履歴） — 根拠: main.cpp:44, Scene.cpp:68, 97, Camera.cpp:12-30; git show MT3_02_03:Structure.cpp 15-27, ab28b04:main.cpp 242-247
- **ImGui::Begin / End** — ウィンドウ "window", "Camera", "Spherical Coordinates", "Window" — 根拠: Scene.cpp:57, 65, 128, 139, 143, 175, Objects.cpp:342, 357
- **ImGui::DragFloat3 / DragFloat** — 速度 0.01f、min/max 付き（Restitution 0..1）、戻り値 bool を変更検知に使用 — 根拠: AABB.cpp:68-71, Sphere.cpp:86-92, Plane.cpp:40
- **ImGui::InputFloat(label, &v, step, step_fast, "%.3f")** — 球面座標入力 — 根拠: Scene.cpp:149-151
- **ImGui::Text（printf 書式）** — ベクトル / 行列の表示、`%zu`、`%8.3f` — 根拠: Scene.cpp:58-64, 166-172, Objects.cpp:325
- **ImGui::TreeNode / TreePop** — 図形種別の折りたたみ、Hierarchy の関節ごと — 根拠: Objects.cpp:319-332, Hierarchy.cpp:55-80
- **ImGui::PushID(int) / PopID** — ループ内の同名ウィジェット識別（`PushID(const char*)` は履歴） — 根拠: Objects.cpp:323-329; git show MT3_02_03:Scene.cpp 84-98
- **ImGui::Button / SameLine** — Start / Reset ボタンを横並び — 根拠: Sphere.cpp:93-103, Spring.cpp:73-75
- **ImGui::Checkbox** — 重力有効化、制御点表示切替 — 根拠: Sphere.cpp:88, Curve.cpp:71
- **ImGui::Separator** — 項目区切り — 根拠: Sphere.cpp:105, Scene.cpp:146
- **ImGui::SliderFloat3 / SliderFloat（履歴）** — 範囲付きスライダ（1615c67 のみ、直後に DragFloat へ変更） — 根拠: git show 1615c67:main.cpp 236-237
- **ImGui::InputFloat3 + ImGuiInputTextFlags_ReadOnly（履歴）** — 読み取り専用表示（MT3_02_00、未コンパイルの `#ifdef ImGui` 内） — 根拠: git show MT3_02_00:main.cpp 208-211
- **imgui.h の include 位置** — 各 .cpp の `#ifdef _DEBUG` 内で `#include <imgui.h>` — 根拠: Sphere.cpp:83-84, Scene.cpp:5-7

---

## E. 判断に迷うもの・備考
- **FrustumDebug / Clipping は定義のみで未使用** — DrawFrustum / CreateFrustum / IsInsideFrustum / NormalizePlane / DrawClippedLine / TransformToClip / ClipLine は全履歴を通じて定義ファイル外から一度も呼ばれていない（8bed0be で追加） — 根拠: 全コミット `git grep` で呼び出し 0 件
- **HEAD で未使用の関数** — ToSpherical（全履歴で未呼出）、Transpose / MakeOrthographicMatrix（MT3_00_02・MT3_01_00 で表示のみ）、Segment::ClosestPoint（MT3_02_00 のデモ以降未使用）、VectorScreenPrintf / MatrixScreenPrintf（MT3_02_02 以降未使用）、Camera::GetMoveSpeed / GetRotateSpeed、Sphere::SetVelocity、AABB::SetMinMax、Triangle::SetVertex、Vector3 の単項 `+`/`-` — 根拠: HEAD grep
- **Vector3::operator[] の実使用箇所** — `half[i]`, `halfA[i]`, `halfB[j]`（const 版） — 根拠: Collision.cpp:250-253, 349, 365
- **main.cpp の ESC 終了判定は死にコード** — main では keys / preKeys を更新しないため（MT3_02_02 以降、入力取得は各 Scene 内） — 根拠: main.cpp:16-17, 44-46
- **raw new の解放漏れ** — `objects = new Objects;` に対応する delete / unique_ptr なし（SceneManager は unique_ptr 化済みで不統一） — 根拠: Scene.cpp:84, Scene.h:65
- **球面座標カメラは表示のみ** — MT4_00_01 の LookAt 行列は ImGui に表示するだけで、実際のビュー行列は Camera::Update のオイラー角版のまま — 根拠: Scene.cpp:112, 159-173
- **初期 VP 行列は毎フレーム上書き** — GameScene コンストラクタの fov 0.50 / far 2000 / viewport maxDepth 2000 は Camera::Update の 0.45 / 100 / 1.0 で上書きされる — 根拠: Scene.cpp:85-86, Camera.cpp:46-48
- **球の分割数の変遷** — MT3_01_02 は 16、MT3_02_02 以降 12 — 根拠: git show ab28b04:main.cpp 153; Sphere.cpp:52
- **線分–平面の平行判定が `dot == 0.0f` の厳密比較**（三角形版は 1e-6 の閾値） — 根拠: Collision.cpp:79, 117
- **EX タグ** — MT3_02_08_EX〜10_EX（OBB 系）と MT3_04_00_EX（ばね）は発展課題扱い — 根拠: git tag 名
- **マージコミット** — MT3_02_05 / 06 / 07、c264d2a、MT3_03_00 (52ee566) はマージコミット（Curve はこのマージで追加） — 根拠: git log --decorate、git show 52ee566 --stat
- **行列規約のメモ** — 行ベクトル・左手系・深度 0..1（DirectX）。列ベクトル流儀の式を混ぜる場合は転置が必要 — 根拠: Matrix4x4.cpp:120-147, 235-248
- **MT3_00_01 の未使用 Vector2 と綴り誤り `Multipry`** — 根拠: git show MT3_00_01:main.cpp 4-6, 19
- **旧スナップショットフォルダ** — repos 直下の MT3_00_01〜MT3_01_02 はタグと同内容（clang-format 前の整形・BOM・タイトルの差のみ） — 根拠: 空白無視 diff 比較
- **コンパイラ依存** — `_In_` SAL 注釈、`WINAPI`、`memcpy` の暗黙 include、`M_PI` は MSVC / Windows 前提 — 根拠: main.cpp:7, Scene.cpp:53, Sphere.cpp:1
- **Curve.cpp 先頭の `#pragma once`** — .cpp には不要（無害） — 根拠: Curve.cpp:1
- **ImGui 依存の Debug 専用 UI** — Release ビルドでは Start ボタンが無いため物理シミュレーションを開始できない（`isStart_` を切り替える手段が ImGui のみ） — 根拠: Sphere.cpp:83-107, Spring.cpp:66-78

---

## このソースで学んだと言える主要トピック（タグ順）
1. MT3_00_01 → Vector3 と基本演算（加減・スカラー倍・内積・長さ・正規化）、Novice::ScreenPrintf での検算表示
2. MT3_00_02 → Matrix4x4 と加減乗・逆行列（ガウス・ジョルダン）・転置・単位行列
3. MT3_00_03 → 平行移動行列・拡大縮小行列・Transform（w 除算、assert）
4. MT3_00_04 → X / Y / Z 回転行列と X→Y→Z の合成
5. MT3_00_05 → アフィン変換行列 S·R·T（ワールド行列）
6. MT3_01_00 → 透視投影・正射影・ビューポート行列
7. MT3_01_01 → クロス積、レンダリングパイプライン（ローカル→ワールド→ビュー(Inverse)→射影→ビューポート）で三角形を描画、WASD 移動
8. MT3_01_02 → デバッググリッド、ワイヤーフレーム球、回転付きカメラ（逆変換の手組み）、ImGui DragFloat3 導入
9. MT3_02_00 → Vector3.h / Matrix4x4.* への分割、正射影ベクトル Project と最近接点 ClosestPoint
10. MT3_02_01 → 球と球の衝突判定、std::vector、キー操作で球を動かしヒットで赤表示
11. MT3_02_02 → クラス化（Scene / SceneManager / Camera / Sphere / Plane）、Collision.cpp、球と平面の判定、平面描画
12. MT3_02_03 → Segment クラス、線分と平面の判定、ImGui PushID / PopID
13. MT3_02_04 → 図形ごとのファイル分割、Triangle、線分と三角形の判定、AABB 描画、SceneManager の unique_ptr 化、_DEBUG ガード
14. MT3_02_05 → AABB 同士の判定、Vector3 演算子（+, −）
15. MT3_02_06 → AABB と球の判定（最近接点 clamp）
16. MT3_02_07 → AABB と線分の判定（スラブ法）、Objects クラスへ集約
17. (8bed0be, MT3_02_07〜08 の間) → OBB クラス（Gram–Schmidt）、OBB と球の判定、Vector4、クリッピング / 視錐台コード（未使用）
18. MT3_02_08_EX → OBB と線分（ローカル空間化）、OBB 同士（SAT 15 軸）、OBB と AABB、ヒットフラグ方式・ジェネリックラムダ・テンプレート
19. MT3_02_09_EX / MT3_02_10_EX → OBB 判定用の配置調整（Objects.cpp のみ変更）
20. MT3_03_00 → 2 次ベジエ曲線（Lerp の 2 段、De Casteljau）
21. MT3_03_01 → 階層構造（肩→肘→手の親子行列）
22. MT3_03_02 → Vector3 / Matrix4x4 の演算子オーバーロード一式と TitleScene での検算表示
23. MT3_04_00_EX → ばね（フックの法則・減衰・オイラー積分・Δt=1/60）
24. MT3_04_01 → 等速円運動（角速度）
25. MT3_04_02 → 単振り子（角加速度 −g/L·sin θ）
26. MT3_04_03 → 円錐振り子（ω = sqrt(g/(L cos φ))）
27. MT3_04_04 → 重力落下と平面での反発（Reflect / Project・反発係数・カプセルによるスイープ判定）
28. MT4_00_01 → 球面座標（ToCartesian / ToSpherical）と注視点カメラ行列（LookAt）
