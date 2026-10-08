# 知識項目インベントリ：AL2_01 / AL2_02 / PG2_Test2（Novice 製の過去作）

## 0. 調査範囲と方法

- 読んだファイル（全文）
  - `C:\Users\s-dai\Downloads\AL2_01`：Collision.cpp/.h, Function.h, Map.cpp/.h, Object.cpp/.h, Particle.cpp/.h, Player.cpp/.h, main.cpp（12 ファイル）
  - `C:\Users\s-dai\Downloads\AL2_02`：main.cpp（1 ファイル）
  - `C:\Users\s-dai\Downloads\PG2_Test2-master`：Bullet.cpp/.h, Camera.cpp/.h, Enemy.cpp/.h, Player.cpp/.h, Scene.cpp/.h, Matrix4x4.cpp/.h, Vector3.h, main.cpp, ReadMe.md, .gitignore, .gitattributes
- `PG2_Test2-master/1_game` の中身は `PG2_Test2.exe` のみで、ソースは無かった。
- 除外：zip / exe / NoviceResources / vcxproj / filters / sln / imgui.ini。
- 行番号はファイル先頭を 1 行目とした実際の行番号（CRLF ファイルも同じ）。
- 「未使用」「どこにも無い」と書いた項目は、3 ソース全体に対する grep で件数 0 を確認したもの（§4 参照）。推測で書いた項目は無い。

---

## 1. AL2_01 — 「レインボーアタック」（2D アクション：マップチップ・3 種の攻撃・パーティクル・カメラ）

### A. C++ 言語機能・標準ライブラリ

- **#pragma once** — 全ヘッダのインクルードガード — 根拠: Function.h:1, Collision.h:1, Object.h:1, Map.h:1, Particle.h:1, Player.h:1
- **#pragma region / #pragma endregion** — コード折りたたみの区切り（日本語名のリージョンも使用） — 根拠: Particle.h:11,33,49,74,76,83; Player.cpp:25,53,55,173,175,196,247,283,411,447
- **宣言(.h)と定義(.cpp)の分離** — クラス・関数ごとにヘッダと実装を分ける — 根拠: Collision.h/Collision.cpp, Object.h/Object.cpp, Map.h/Map.cpp, Particle.h/Particle.cpp, Player.h/Player.cpp
- **struct（データ集成体）** — Vector2 / Gravity / Camera / Circle / Rect / Overlap / AfterImageParticle をデータの入れ物として定義 — 根拠: Function.h:3-22, Collision.h:4-16, Particle.h:52-58
- **struct のデフォルトメンバ初期化子** — Camera の shakeX/shakeY/shakeTimer に既定値 — 根拠: Function.h:19-21
- **波括弧による集成体初期化・代入** — `rect_ = { x, y, size, size }`、`Camera camera{ 0, 0, w, h }`、`return { 100.0f, 100.0f }` — 根拠: Object.cpp:4, main.cpp:73, Map.cpp:12-17,63,67, Player.cpp:16-17,183,185-186
- **class と private / public / protected** — Object, Map, Particle, ParticleEmitter（protected）, PlayerAttack, Player — 根拠: Object.h:5-11, Particle.h:13-21,35-40, Player.h:9-19,30-39, Map.h:7-8,52
- **コンストラクタのデフォルト引数** — `Object(float x, float y, float size = 64.0f)`、Particle の size/color/shape — 根拠: Object.h:12, Particle.h:22
- **デストラクタでの所有リソース解放** — Object/Map/Player/ParticleEmitter が delete を行う — 根拠: Object.cpp:9-11, Map.cpp:52-57, Player.cpp:191-194, Particle.cpp:52-56
- **メンバ初期化子リスト** — `ParticleEmitter() : particleCount_(0)`、`PlayerEmitter() : afterImageColorIndex_(0), writeIndex_(0)` — 根拠: Particle.cpp:46, Particle.cpp:119-120
- **public 継承** — PlayerEmitter / ObjectEmitter が ParticleEmitter を継承 — 根拠: Particle.h:60, Particle.h:78
- **仮想関数・仮想デストラクタ** — ParticleEmitter の AddParticle/Update/Draw を virtual、`virtual ~ParticleEmitter()` — 根拠: Particle.h:42-46
- **protected メンバ** — 派生エミッタから particles_ / particleCount_ を使えるようにする — 根拠: Particle.h:36-38
- **const メンバ関数** — Getter 系（GetRect, IsAlive, IsAttacking, GetFacing, GetRectCount, GetPlayerStartPosition など） — 根拠: Object.h:16-17, Particle.h:25-26, Player.h:24-27,48,55,59-62, Map.h:57,60,63
- **クラス内定義のインライン関数（Setter/Getter）** — `SetColor`, `SetSize`, `SetShape`, `GetRects()`, `GetObjects()` — 根拠: Particle.h:28-30, Map.h:56-57,62-63
- **const 参照の戻り値** — `const Rect& GetRect() const` — 根拠: Object.h:16, Player.h:48, Player.cpp:198
- **参照引数（const& と非 const&）** — 当たり判定の `const Circle&`、`Player&` / `Camera&` を書き換え用に受ける、`bool &position` — 根拠: Collision.h:19-25, Player.h:22,44-45, main.cpp:16
- **ポインタによる出力引数** — `Overlap* overlap` に判定結果を書き込む — 根拠: Collision.h:25, Collision.cpp:42-43
- **前方宣言** — `class Map; class Player;` で相互参照を解決 — 根拠: Player.h:6-7
- **new / delete（単体オブジェクト）** — Emitter / Attack / Map / Player を動的確保・解放 — 根拠: Object.cpp:6,10; Player.cpp:179-180,192-193; main.cpp:63,66,187-188
- **new[] / delete[]（ポインタ配列）** — `objects_ = new Object*[objectCount_]`、`delete[] objects_` — 根拠: Map.cpp:36, Map.cpp:56
- **ダブルポインタ（Object**）** — 敵配列の受け渡し — 根拠: Map.h:49,62, main.cpp:69, Player.cpp:346
- **ポインタの固定長配列と nullptr** — `Particle* particles_[100]` を nullptr で初期化 — 根拠: Particle.h:37, Particle.cpp:47-49
- **クラス内 static const int 定数** — `static const int MAP_WIDTH = 40;` — 根拠: Map.h:9-10
- **グローバル const 定数と定数式での計算** — SCREEN_WIDTH, TILE_SIZE, `MAP_PIXEL_WIDTH = MAP_WIDTH * TILE_SIZE` — 根拠: Function.h:25-33
- **2 次元配列メンバのクラス内初期化** — `int mapData_[MAP_HEIGHT][MAP_WIDTH] = { ... }` — 根拠: Map.h:23-47
- **構造体の 1 次元配列メンバ** — `Rect rects_[MAP_WIDTH * MAP_HEIGHT]` — 根拠: Map.h:20
- **非スコープ enum** — `enum MapNumber { _, G, P, E }`（マップ表の記号）、`enum ParticleShape { Box, Circles }` — 根拠: Map.h:12-17, Particle.h:6-9
- **enum 値の修飾アクセス** — `ParticleShape::Box`（非スコープ enum でも型名で修飾） — 根拠: Particle.h:22, Particle.cpp:30,34,95,104
- **static_cast** — int→float、float→int の明示変換 — 根拠: Map.cpp:13-14,42-43,63; Particle.cpp:92,102-103,149; Player.cpp:148,151,157-158; main.cpp:132-134,144-145,172
- **C スタイルキャスト・関数形式キャスト** — `(int)sx`, `(float)strength`, `float(M_PI)` — 根拠: Object.cpp:35, Player.cpp:122-123, main.cpp:73,103
- **<algorithm> の std::min / std::max** — 円-矩形判定の最近点クランプ — 根拠: Collision.h:2, Collision.cpp:6-7
- **`(std::min)(a, b)` の括弧付き呼び出し** — Windows.h の min マクロとの衝突回避 — 根拠: Player.cpp:120
- **<math.h> と _USE_MATH_DEFINES / M_PI** — `cos`, `sin`, `sinf`, `M_PI` — 根拠: Player.cpp:6-7,148,151,157-158; main.cpp:2,102-103
- **rand() と剰余による乱数範囲** — `rand() % 11 - 5`、`(rand() % 100) / 100.0f` で小数のばらつき — 根拠: Particle.cpp:92,102-103; Player.cpp:365-366,371-372,385-386,391-392,435-436
- **double 型の使用** — 円同士の距離計算とイージング — 根拠: Collision.cpp:19-22, main.cpp:8-28,50-53
- **memcpy** — 前フレームのキー配列を保存 — 根拠: main.cpp:81
- **uint32_t / unsigned int** — 色値の保持と変換 — 根拠: Particle.h:18, Particle.cpp:164-172
- **ビット演算（& | << >>）** — アルファ値の差し替え、ARGB→RGBA の並べ替え — 根拠: Particle.cpp:150, Particle.cpp:164-172
- **三項演算子** — `right ? 1.0f : -1.0f`、寿命比率、開始角度 — 根拠: Player.cpp:253,151; Particle.cpp:148
- **早期 return / continue のガード節** — 非アクティブ要素のスキップ、攻撃中でなければ抜ける — 根拠: Object.cpp:32, Particle.cpp:13,27,146,162, Player.cpp:28,59,261,298,351
- **ファイルスコープ static const 配列** — 残像用 7 色テーブル — 根拠: Particle.cpp:109-117
- **配列要素への参照エイリアス** — `AfterImageParticle& p = afterImages_[writeIndex_];` — 根拠: Particle.cpp:127
- **剰余によるリングバッファ添字** — `(writeIndex_ + 1) % 100`、`(afterImageColorIndex_ + 1) % 7` — 根拠: Particle.cpp:137,140
- **グローバル変数（非 const）** — `int totalFrames = 60; int currentFrame = 0;` — 根拠: main.cpp:13-14
- **配列を引数に取る関数** — `void UpdateEase(double posX[], bool &position, bool &isEasing)` — 根拠: main.cpp:16
- **`const char kWindowTitle[]`** — ウィンドウタイトル文字列定数（日本語を含む） — 根拠: main.cpp:37
- **WinMain エントリポイント（SAL 注釈付き）** — `int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)` — 根拠: main.cpp:40
- **`*this` の受け渡し** — `attack_->Update(*this, ...)` — 根拠: Player.cpp:342
- **構造体の値コピー** — `Rect rect = player.GetRect();` — 根拠: Player.cpp:62,414
- **bool と int / char の相互変換** — `bool guidePosition = 0;`、`keys[DIK_S] != 0`、`bool right = (keys[DIK_D]);` — 根拠: main.cpp:55, Player.cpp:250-251,339
- **ヘッダから <Novice.h> を include** — Particle.h が Novice.h に依存（これを include する全ファイルが Novice 依存） — 根拠: Particle.h:4
- **複合代入演算子** — `*=`、`+=` — 根拠: Player.cpp:202,258,290,295
- **ローカル const 定数** — `const double c1 = 1.70158; const double c3 = c1 + 1.0;` — 根拠: main.cpp:9-10

### B. 数学・物理・当たり判定

- **2D ベクトル構造体** — Vector2 を位置・速度・加速度・移動方向に使用 — 根拠: Function.h:3-6, Function.h:9-14, Player.cpp:241
- **重力（速度 += 加速度）と終端速度** — 毎フレーム vy += 0.5、上限 15 — 根拠: Player.cpp:185-186,290-294
- **ジャンプ（接地時に負の初速）** — `velocity.y = -15.2f`、同時に onGround=false — 根拠: Player.cpp:318-323
- **パーティクルの重力** — `velocity_.y += 0.3f` — 根拠: Particle.cpp:16
- **軸分離の移動＋押し戻し** — X 方向に動かして判定→押し戻し、次に Y 方向に動かして判定→押し戻し — 根拠: Player.cpp:258-273, 295-315
- **速度符号による着地 / 天井判定** — vy>0 で着地（上面に合わせ onGround=true）、vy<0 で天井（下面に合わせる） — 根拠: Player.cpp:303-312
- **毎フレーム onGround を false にリセット** — 空中ジャンプの防止 — 根拠: Player.cpp:286-288
- **AABB（矩形同士）判定** — 左右上下の境界比較、X/Y 別の重なりフラグを返す — 根拠: Collision.cpp:28-46
- **円と矩形の判定（最近点法）** — clamp で矩形内の最近点を求め、距離² ≤ r² — 根拠: Collision.cpp:4-15
- **円同士の判定** — 距離² ≤ (r1+r2)² — 根拠: Collision.cpp:18-25
- **sqrt を避けて距離の二乗で比較** — 上記 2 関数 — 根拠: Collision.cpp:12-14,21-24
- **三角関数による円運動（回転攻撃の判定位置）** — 40F で 2 回転、向きで開始角を切替、`center + r*cos(angle)` / `sin` — 根拠: Player.cpp:146-158
- **sin 波による上下の揺れ** — `sinf(theta) * amplitude + 16`、theta += π/53 — 根拠: main.cpp:58-60,102-103
- **イージング（EaseInBack）** — `c3*x³ - c1*x²`（c1=1.70158） — 根拠: main.cpp:7-12
- **正規化時間 t と線形補間** — `t = currentFrame/totalFrames`、`start + (end-start)*ease`、終了で t=1 にクランプ — 根拠: main.cpp:16-34
- **寿命比率によるアルファフェード** — `alpha = 255 * life/30` を色の上位バイトへ — 根拠: Particle.cpp:148-150
- **ワールド→スクリーン変換（カメラ減算）** — `screenX = worldX - camera.x` — 根拠: Object.cpp:33-34, Particle.cpp:28-29,174-175, Player.cpp:222-223, main.cpp:124-125,132-134
- **カメラ追従（プレイヤー中心合わせ）** — `camera.x = centerX - camera.width/2` — 根拠: Player.cpp:416-422
- **カメラのマップ内クランプ** — 0 以上、マップ端−画面幅以下 — 根拠: Player.cpp:424-430
- **カメラシェイク（±強度の乱数オフセット＋タイマー）** — `rand() % (2s+1) - s` を shakeTimer の間だけ加算 — 根拠: Player.cpp:432-441
- **シェイク強度を攻撃継続時間から算出** — `min(attackFrame/5, 40)`、30F 揺らす — 根拠: Player.cpp:117-123
- **プレイヤー位置のマップ端クランプ** — 0 と MAP_PIXEL_WIDTH − 幅 — 根拠: Player.cpp:275-281
- **速度の上限クランプ** — LimitHorizontalSpeed（±maxSpeed） — 根拠: Player.cpp:20-23
- **パーティクルのばらつき（乱数速度）** — 横速度にランダム、左右 2 方向へ分けて噴出 — 根拠: Particle.cpp:90-106, Player.cpp:363-374,383-394
- **タイル座標⇔ピクセル座標** — `index = y*W + x`、`px = x*TILE_SIZE` — 根拠: Map.cpp:9,13-14,42-43,63
- **矩形の中心座標** — `leftTop + size/2` — 根拠: Object.cpp:22-23, Player.cpp:63-64,417-418
- **色 0xRRGGBBAA（Novice）と内部 ARGB の相互変換** — alpha を bit24-31 に置いて保持し、描画時に RGBA へ並べ替え — 根拠: Particle.cpp:150,164-172
- **カメラ外パーティクルの消滅（簡易カリング）** — カメラ矩形外で isAlive=false — 根拠: Particle.cpp:17-23
- **反対キー同時押しの相殺** — `right != left` のときだけ移動 — 根拠: Player.cpp:250-255

### C. ゲームの構造・実装パターン

- **Novice のゲームループ骨格** — Initialize → `while (ProcessMessage() == 0) { BeginFrame; 入力; 更新; 描画; EndFrame; }` → Finalize — 根拠: main.cpp:43-191
- **keys / preKeys の二重バッファ入力** — memcpy で前フレームを保存し GetHitKeyStateAll で更新 — 根拠: main.cpp:46-47,81-82
- **押した瞬間（トリガー）判定** — `keys[X] && !preKeys[X]` — 根拠: Player.cpp:110,318,326,402; main.cpp:91,182
- **押しっぱなし判定** — `keys[DIK_D]` をそのまま bool として使用 — 根拠: Player.cpp:250-251,328,331,339
- **ESC で終了** — `preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0` で break — 根拠: main.cpp:182-184
- **責務ごとのクラス分割** — Player / PlayerAttack / Map / Object / Particle / ParticleEmitter / PlayerEmitter / ObjectEmitter — 根拠: Player.h:9,30; Map.h:7; Object.h:5; Particle.h:13,35,60,78
- **所有ポインタによる包含（コンポジション）** — Player が PlayerAttack* と PlayerEmitter*、Object が ObjectEmitter* を所有 — 根拠: Player.h:34-35, Object.h:9
- **継承による機能拡張（エミッタ）** — 基底にプール管理、派生に用途別の発生関数（AddAttackParticles / AddDestroyParticles / 残像） — 根拠: Particle.h:35-81, Particle.cpp:90-106,126-186
- **Update / Draw のメンバ関数分離** — 各クラスが Update(...) と Draw(const Camera&) を持つ — 根拠: Object.h:20-21, Player.h:44-46, Particle.h:24-25,45-46
- **マップを int 二次元配列＋enum 記号で記述** — `_ G P E` を並べて可読性を確保 — 根拠: Map.h:12-47
- **マップデータ→当たり判定矩形配列への変換** — ブロックは Rect、空白は幅 0 で無効化 — 根拠: Map.cpp:6-23
- **幅 0 判定で空タイルをスキップ** — `if (rects[i].width == 0) continue;` — 根拠: Player.cpp:261,298; main.cpp:123
- **マップからプレイヤー初期位置を取得** — 値 2 を探索、無ければ既定値 (100,100) — 根拠: Map.cpp:59-68, Player.cpp:182-183,233
- **マップからの敵（Object）配置** — 値 3 を数えて配列確保→生成（2 パス） — 根拠: Map.cpp:25-48
- **生存フラグによるオブジェクト管理** — isAlive_、Kill()、ResetAlive() — 根拠: Object.h:8,15,18; Object.cpp:16-25
- **固定長プールと空きスロットの再利用** — 100 個の Particle* 配列、死んだスロットは delete→new で再初期化 — 根拠: Particle.cpp:58-71
- **リングバッファ（残像）** — writeIndex を巡回させ最古を上書き — 根拠: Particle.h:61-63, Particle.cpp:126-141
- **寿命カウントダウンと消滅** — life-- で 0 以下なら非アクティブ — 根拠: Particle.cpp:144-157
- **色テーブルの巡回** — 7 色配列を添字でループ（虹色の残像） — 根拠: Particle.cpp:108-117,132,137
- **bool フラグ＋フレームカウンタによる攻撃の状態機械** — isAttack_/isSpin_/isDive_ と attackFrame_ の閾値（17/18/22/40） — 根拠: Player.h:13-17, Player.cpp:27-51,58-166
- **3 種類の攻撃の切り替え（地上 / 空中回転 / 急降下）** — 接地状態と S/下キーの有無で分岐 — 根拠: Player.cpp:34-50, 326-341
- **急降下攻撃の演出手順** — 17F 上昇→急降下（vy=18）→横速度制限→残像追加→着地で解除＋シェイク＋パーティクル、W で途中解除 — 根拠: Player.cpp:66-133
- **向き（facing）の記憶** — 移動入力で ±1 を更新、攻撃時に入力が無ければ最後の向きを使う — 根拠: Player.cpp:252-255,327-337
- **攻撃判定とオブジェクトの衝突処理** — 急降下は矩形、他は円で判定し Kill＋破片パーティクル — 根拠: Player.cpp:349-399
- **カメラ構造体とシェイク用フィールド** — Camera{x, y, width, height, shakeX, shakeY, shakeTimer} — 根拠: Function.h:17-22
- **更新→カメラ更新の順序** — `player->Update(...)` の後に `player->UpdateCamera(...)` — 根拠: main.cpp:89-90
- **R キーでリセット** — プレイヤー位置復帰と全 Object の復活 — 根拠: Player.cpp:401-407
- **描画順序の管理** — マップ→外周ブロック→敵→デバッグ線→プレイヤー→攻撃判定→ガイド UI — 根拠: main.cpp:119-172
- **デバッグ表示（タイルグリッド線・攻撃円のワイヤ表示）** — DrawLine でグリッド、kFillModeWireFrame で円 — 根拠: main.cpp:141-147,152-170
- **UI ガイド画像のアニメーション** — 右キーで左右位置をイージング移動、sin で上下に揺らす — 根拠: main.cpp:49-60,91-107,172
- **main での動的生成と終了時解放** — `new Map`, `new Player(map)` → delete — 根拠: main.cpp:63-66,187-188
- **破壊エフェクトを所有エミッタに委譲** — Object::Kill → emitter_->AddDestroyParticles — 根拠: Object.cpp:20-25
- **エミッタの更新 / 描画を所有者が駆動** — Object::Draw 内で emitter_->Update / Draw、Player::UpdateCamera 内で emitter_->Update — 根拠: Object.cpp:30-39, Player.cpp:443

### D. 使用しているフレームワーク API（Novice）

- **Novice::Initialize(title, width, height)** — 1280x720 で初期化 — 根拠: main.cpp:43
- **Novice::ProcessMessage()** — 0 以外で終了 — 根拠: main.cpp:76
- **Novice::BeginFrame() / Novice::EndFrame()** — フレームの開始 / 終了 — 根拠: main.cpp:78,179
- **Novice::GetHitKeyStateAll(char*)** — 256 キーの押下状態を取得 — 根拠: main.cpp:82
- **Novice::Finalize()** — 終了処理 — 根拠: main.cpp:191
- **Novice::LoadTexture(path)** — "./Guide.png" を読み込みハンドルを得る — 根拠: main.cpp:49
- **Novice::DrawSprite(x, y, handle, scaleX, scaleY, angle, color)** — ガイド画像描画 — 根拠: main.cpp:172
- **Novice::DrawBox(x, y, w, h, angle, color, fillMode)** — プレイヤー / 敵 / マップ / パーティクル / 残像の矩形 — 根拠: Object.cpp:35-36, Particle.cpp:31-33,176-184, Player.cpp:224-226, main.cpp:126-134
- **Novice::DrawEllipse(cx, cy, rx, ry, angle, color, fillMode)** — 円パーティクルと攻撃円 — 根拠: Particle.cpp:35-39, main.cpp:164-168
- **Novice::DrawLine(x1, y1, x2, y2, color)** — デバッググリッド — 根拠: main.cpp:144-145
- **kFillModeSolid / kFillModeWireFrame** — 塗りつぶし / ワイヤの切替 — 根拠: Object.cpp:36, Particle.cpp:33,39, main.cpp:128,167
- **色定数 RED** — プレイヤー描画色 — 根拠: Player.cpp:226
- **色指定 0xRRGGBBAA** — 0x00FF00FF, 0x306230FF, 0xFFA500FF, 0x00EEFFFF など — 根拠: Object.cpp:36, main.cpp:128,167, Particle.cpp:95,104,110-116
- **DIK_ キーコード** — DIK_A / D / W / S / UP / DOWN / LEFT / RIGHT / R / ESCAPE — 根拠: Player.cpp:110,250-251,318,326,328,331,339,402; main.cpp:91,182

### E. 判断に迷うもの・備考

- **Update 内での描画呼び出し** — Player::Update が DrawAfterImages を呼び、Object::Draw が emitter_->Update を呼ぶ（更新 / 描画の分離が崩れている箇所） — 根拠: Player.cpp:343-344, Object.cpp:31
- **マジックナンバーでのマップ値比較** — enum MapNumber を定義しつつ比較は 1 / 2 / 3 — 根拠: Map.h:12-17 vs Map.cpp:10,29,40,62
- **同名定数の二重定義** — Map::MAP_WIDTH / MAP_HEIGHT（クラス内）と Function.h のグローバル MAP_WIDTH / MAP_HEIGHT — 根拠: Map.h:9-10, Function.h:29-30
- **定義されているが未使用** — IsCircleCollidingWithCircle、Player::ScaleVelocityX、Player::StartCameraShake、Particle::SetColor / SetSize / SetShape、Object::Update（空実装） — 根拠: Collision.cpp:18-25, Player.cpp:202,212-216, Particle.h:28-30, Object.cpp:27-28
- **Gravity::acceleration.x は常に 0 で未使用** — 根拠: Player.cpp:186
- **srand 未使用** — 乱数列は毎回同じ（grep で srand 0 件）
- **カメラシェイクはクランプ後に加算** — シェイク中はマップ外にはみ出し得る — 根拠: Player.cpp:425-441
- **UpdateCamera がメンバ関数なのに Player& を引数で受ける** — `player->UpdateCamera(camera, *player)` — 根拠: Player.h:45, main.cpp:90
- **PlayerAttack が Player& を受け取り速度を直接書き換える** — 密結合な設計（SetVelocityY / LimitHorizontalSpeed 呼び出し） — 根拠: Player.cpp:58,70,73,77,111,118
- **コメントアウトされた旧コードが残存** — 急降下判定の向き別矩形、デバッグ矩形描画、背景塗り — 根拠: Player.cpp:84-85,89-101; main.cpp:117,154-162,169
- **プレイヤー・敵の描画は矩形のみ（テクスチャ無し）** — スプライトはガイド UI だけ — 根拠: Player.cpp:224-226, Object.cpp:35-36, main.cpp:172
- **std::vector / スマートポインタは未使用** — 固定長配列と生ポインタのみ（AL2_01 内で vector / unique_ptr 0 件）
- **ソースは UTF-8（BOM 付き）・CRLF** — 日本語コメント・日本語タイトルを含む — 根拠: main.cpp:37, Player.cpp:25（リージョン名）

### このソースで学んだと言える主要トピック

- マップチップ（int 二次元配列）から当たり判定矩形を生成し、軸分離の押し戻しで実装するプラットフォーマー物理（重力・終端速度・ジャンプ・着地・天井）
- 矩形同士 / 円と矩形 / 円同士の当たり判定の自作（距離の二乗比較、最近点法）
- bool フラグ＋フレームカウンタによる攻撃の状態管理（地上 / 空中回転 / 急降下）とフレーム閾値での演出制御
- 三角関数による円運動、EaseInBack イージング、sin 波の揺れ
- カメラ（追従・マップ内クランプ・乱数シェイク）とワールド→スクリーン変換（カメラ座標の減算）
- 固定長プール・リングバッファによるパーティクル / 残像管理、寿命によるアルファフェード、色のビット操作（ARGB⇔RGBA）
- クラス設計：所有ポインタによるコンポジション、継承と仮想関数（エミッタ）、const メンバ関数、前方宣言、Getter/Setter
- 生ポインタと new / delete / new[] / delete[] による手動メモリ管理、ダブルポインタ配列
- Novice の描画 API（DrawBox / DrawEllipse / DrawLine / DrawSprite）と keys / preKeys によるトリガー入力

---

## 2. AL2_02 — ステージデータテーブルと敵スポーン（ImGui 調整 UI 付き）

### A. C++ 言語機能・標準ライブラリ

- **<assert.h> と assert** — プール枯渇時 `assert(0 && "No inactive enemy slots available!")`、範囲チェック `assert(0 <= Index && Index < _countof(stageData))` — 根拠: main.cpp:2,54,187
- **条件コンパイル #ifdef / #endif** — `#ifdef USE_IMGUI` で ImGui.h の include を切替 — 根拠: main.cpp:3-5
- **std::string / std::to_string / c_str() / 文字列連結（+）** — ImGui のラベルを動的に生成 — 根拠: main.cpp:6,277-278,282,289
- **std::vector<T> をメンバに持つ struct** — `std::vector<EnemySpawnData> enemySpawnDatas;` — 根拠: main.cpp:7,139
- **入れ子の集成体初期化（struct 配列の中に vector）** — ステージ表全体を波括弧で一括初期化 — 根拠: main.cpp:145-181
- **struct のデフォルトメンバ初期化子** — Enemy の isActive=false / position={-128,-128} / scale=1.0f、Player の position、StageData の timeLimit=0 — 根拠: main.cpp:17-21,76-78,135-142
- **const int 定数（k プレフィックス命名）** — `const int kEnemyMax = 100;` — 根拠: main.cpp:23
- **グローバル配列・グローバルインスタンス・グローバル状態** — `Enemy enemies[kEnemyMax]; Player player; StageData stageData[]; int stageIndex; int prevIndex;` — 根拠: main.cpp:25,86,145,183,208
- **参照渡しと値渡しの使い分け** — `Enemy& enemy`（書き換え対象）、`EnemySpawnData spawnData`（コピー） — 根拠: main.cpp:34,44,89
- **範囲 for（const 参照）** — `for (const EnemySpawnData& spawnData : stageData[stageIndex].enemySpawnDatas)` — 根拠: main.cpp:201
- **_countof マクロ** — 配列要素数の取得 — 根拠: main.cpp:187
- **static_cast<int>** — 描画座標の float→int 変換 — 根拠: main.cpp:67,127
- **sqrtf** — 入力ベクトルの長さ — 根拠: main.cpp:108
- **XML ドキュメントコメント `/// <summary>` `<param>`** — 関数の説明 — 根拠: main.cpp:40-43,57-60,93-96,122-125,192-194
- **#pragma region / endregion** — データ構造ごとの区切り — 根拠: main.cpp:14,71,73,130,132,206
- **continue / 早期 return** — 非アクティブ要素のスキップ、1 体発生したら終了 — 根拠: main.cpp:46-51,63-65,273-274
- **const ローカル変数** — `const int zombieTexture = Novice::LoadTexture(...)` — 根拠: main.cpp:245-246
- **構造体メンバのアドレスを float 配列として渡す** — `&player.position.x` / `&enemies[i].position.x` を SliderFloat2 へ（x, y が連続配置である前提） — 根拠: main.cpp:264,283
- **関数から struct を値で返す** — `StageData GetStageData(int Index)`（vector を含むコピー） — 根拠: main.cpp:185-190
- **前フレーム値との比較による変化検知関数** — `bool IsUpdateIndex(int)` が prevIndex と比較して更新 — 根拠: main.cpp:208-214
- **memcpy** — 前フレームのキー保存 — 根拠: main.cpp:254
- **WinMain / `const char kWindowTitle[]`** — 根拠: main.cpp:227,230
- **char 同士の比較で入力を判定** — `keys[DIK_RIGHT] != keys[DIK_LEFT]` — 根拠: main.cpp:100,104
- **関数の引数に char* でキー配列を渡す** — `void UpdatePlayer(char* keys)` — 根拠: main.cpp:97

### B. 数学・物理・当たり判定

- **入力ベクトルの正規化** — `length = sqrtf(x²+y²)`、0 でなければ各成分を割る（斜め移動の速度を一定に） — 根拠: main.cpp:98-112
- **移動量 = 正規化方向 × 速さ** — `position += moveVec * 5.0f` — 根拠: main.cpp:114-115
- **画面内クランプ（スプライトサイズ考慮）** — x: 0〜1280−98、y: −20〜720−128 — 根拠: main.cpp:116-119
- **スケール値による拡大描画** — DrawSprite の scaleX / scaleY に enemy.scale を渡す — 根拠: main.cpp:67
- **反対キー同時押しの相殺** — `keys[RIGHT] != keys[LEFT]` のときだけ移動 — 根拠: main.cpp:100-105
- （このソースに当たり判定・重力・カメラは無い）

### C. ゲームの構造・実装パターン

- **isActive フラグによるオブジェクトプール** — 固定長配列から非アクティブを探して初期化、満杯なら assert — 根拠: main.cpp:44-55
- **Initialize / Update / Draw の関数分割（非クラス・関数ベース）** — EnemyInitialization / SpawnEnemy / DrawAllEnemies / PlayerInitialization / UpdatePlayer / DrawPlayer — 根拠: main.cpp:34,44,61,89,97,126
- **データ駆動のステージテーブル** — StageData{プレイヤー初期位置, 敵スポーン vector, 制限時間} の配列を 3 ステージ分記述 — 根拠: main.cpp:135-181
- **スポーンデータ構造体と実体の分離** — EnemySpawnData / PlayerSpawnData → Enemy / Player に展開 — 根拠: main.cpp:28-31,81-83
- **テーブルからの一括スポーン** — EnemyDataTableSpawn が範囲 for で SpawnEnemy、SpawnPlayer が初期位置を設定 — 根拠: main.cpp:195-204
- **ステージ切替の検知→プールリセット→再スポーン** — IsUpdateIndex が真なら全敵を非アクティブ化・初期値に戻し、再配置、プレイヤーも再配置 — 根拠: main.cpp:208-214,299-312
- **ImGui によるデバッグ / 調整 UI** — ステージ番号・プレイヤー座標・各敵の座標とスケールをスライダーで編集 — 根拠: main.cpp:261-297
- **動的ラベル生成** — 敵ごとに "Enemy N Position" / "Enemy N Scale" — 根拠: main.cpp:277-278
- **背景グリッドの描画関数** — 白塗り＋10px 間隔の縦横線 — 根拠: main.cpp:217-225
- **描画順序** — 背景→プレイヤー→敵 — 根拠: main.cpp:322-324
- **ゲームループ骨格・keys / preKeys・ESC 終了** — AL2_01 と同じ構成 — 根拠: main.cpp:233-341
- **起動時の初期スポーン** — ループ前に EnemyDataTableSpawn / SpawnPlayer — 根拠: main.cpp:236-239
- **画面外初期位置（−128, −128）で「非表示」を表現** — 根拠: main.cpp:19,77,303
- **ステージ番号の範囲制限をスライダー側で保証** — SliderInt(0..2) — 根拠: main.cpp:263

### D. 使用しているフレームワーク API（Novice / ImGui）

- **Novice::Initialize / ProcessMessage / BeginFrame / EndFrame / GetHitKeyStateAll / Finalize** — 根拠: main.cpp:233,249,251,331,255,340
- **Novice::LoadTexture("./NoviceResources/….png")** — Novice 同梱素材（zombie / maleAdventurer）を使用 — 根拠: main.cpp:245-246
- **Novice::DrawSprite(x, y, handle, scaleX, scaleY, angle, color)** — スケール付き描画 — 根拠: main.cpp:67,127
- **Novice::DrawBox / Novice::DrawLine** — 背景 — 根拠: main.cpp:218-222
- **ImGui::Begin(name) / ImGui::End()** — デバッグウィンドウ — 根拠: main.cpp:261,266,270,297
- **ImGui::SliderInt / SliderFloat / SliderFloat2 / Separator** — 根拠: main.cpp:263,264,281-285,288-292,294
- **DIK_RIGHT / LEFT / DOWN / UP / ESCAPE** — 根拠: main.cpp:100-105,334

### E. 判断に迷うもの・備考

- **ImGui 呼び出し自体は #ifdef で囲まれていない** — include だけ条件付き（USE_IMGUI 未定義ならビルド不可） — 根拠: main.cpp:3-5 vs 261-297
- **未使用の定義** — GetStageData（assert 付き取得関数）、StageData::timeLimit — 根拠: main.cpp:141,185-190
- **ウィンドウタイトルが "AL3_5_1_04_suzuki"** — フォルダ名は AL2_02 だが AL3 課題の可能性がある — 根拠: main.cpp:227
- **SliderFloat2 に &position.x を渡す書き方はメモリレイアウト依存** — 根拠: main.cpp:264,283
- **当たり判定・ゲームオーバー・スコアは未実装** — 配置とパラメータ調整に特化した課題 — 根拠: main.cpp 全体
- **std::vector はデータテーブル内のみ** — 敵の実体は固定長グローバル配列 — 根拠: main.cpp:25,139
- **struct Vector2 を再定義（AL2_01 の Function.h と同形）** — 根拠: main.cpp:9-12

### このソースで学んだと言える主要トピック

- データ駆動設計：ステージテーブル（struct 配列＋std::vector）からプレイヤー・敵をスポーンする
- isActive フラグ式オブジェクトプールと、assert による前提条件チェック
- 入力ベクトルの正規化（斜め移動の速度統一）と画面内クランプ
- ImGui を使ったパラメータ調整 UI（Begin/End、Slider 系、std::string での動的ラベル）
- 範囲 for、std::vector、std::string / to_string、_countof、条件コンパイル
- 前フレーム値との比較による変化検知（ステージ切替処理）
- Novice の LoadTexture / DrawSprite（スケール指定）、矩形と線による背景描画

---

## 3. PG2_Test2 — 3D 表示シューティング（シーン管理・自作行列パイプライン）

### A. C++ 言語機能・標準ライブラリ

- **#pragma once** — 全ヘッダ — 根拠: Vector3.h:1, Matrix4x4.h:1, Camera.h:1, Bullet.h:1, Player.h:1, Enemy.h:1, Scene.h:1
- **#pragma region / endregion（無名リージョン・入れ子を含む）** — 根拠: Matrix4x4.h:12,30,32,41,43,66,68,77-78; Matrix4x4.cpp:5,107,109,111,150,152,252,254,304,306; Scene.cpp:22,44,93,140,161,188,335; Enemy.cpp:110,156
- **POD struct（Vector2 / Vector3 / Matrix3x3 / Matrix4x4 / Sphere）** — 行列は `float m[4][4]` を持つ構造体 — 根拠: Vector3.h:3-9, Matrix4x4.h:4-10, Scene.h:21-24
- **行列の平坦な波括弧初期化（ブレース省略）** — `Matrix3x3 result = { 2/(r-l), 0, ..., 1 };` — 根拠: Matrix4x4.cpp:8-12,17-21,26-30,37-41,46-50
- **値初期化 `{}`** — `Matrix4x4 result{};`、`Matrix3x3 result = {};`、`Vector3 result{};`、`Vector3 rotate{};` — 根拠: Matrix4x4.cpp:55,115,130,141,172,258,275; Player.h:35
- **関数オーバーロード（引数型で 3x3 / 4x4 を切替）** — Multiply / Transform / MakeScaleMatrix / MakeTranslateMatrix / MakeOrthographicMatrix / MakeViewportMatrix / MakePipeline — 根拠: Matrix4x4.h:14-26 と 35-75
- **非メンバ関数による数学ライブラリ** — ヘッダ宣言＋cpp 定義の自由関数群（Lerp / EaseInOut / DrawGrid なども自由関数） — 根拠: Matrix4x4.h, Matrix4x4.cpp, Scene.h:13-29, Scene.cpp:5-20
- **const 参照引数・値返し** — `Matrix4x4 Multiply(const Matrix4x4&, const Matrix4x4&)`、`Vector3 Lerp(const Vector3&, const Vector3&, float)` — 根拠: Matrix4x4.h:35,50, Scene.cpp:5
- **<cmath>（std::cos / std::sin / std::tan / std::sqrt / std::fabs）と C 関数（cosf / sinf / fabsf）** — 根拠: Matrix4x4.cpp:2,35-36,187-188,261; Player.cpp:2,14; Enemy.cpp:2,80,122,144; Scene.cpp:3,63,81,113-125
- **_USE_MATH_DEFINES を <cmath> の前に #define → M_PI** — ヘッダ（Scene.h）側で定義 — 根拠: Scene.h:2-3, Scene.cpp:101-105
- **<assert.h> / assert** — 行列式 0・w=0 のチェック — 根拠: Matrix4x4.cpp:3,74,96
- **<stdlib.h> の rand() / RAND_MAX** — `rand() / RAND_MAX` で 0〜1 の float を得る — 根拠: Enemy.cpp:3,14,22; Scene.cpp:284-286
- **class（public を先に書くスタイル）** — Camera / Bullet / Player / Enemy — 根拠: Camera.h:5-29, Bullet.h:5-26, Player.h:6-52, Enemy.h:6-45
- **クラス内デフォルトメンバ初期化子（NSDMI）／波括弧初期化** — `Vector3 position_{ 0.0f, 0.0f, 0.0f }; float speed_ = 5.0f;` — 根拠: Camera.h:25-28, Bullet.h:23-25, Player.h:34-38, Enemy.h:33-42, Scene.h:79-109
- **メンバ配列の NSDMI（他メンバを参照）** — `Vector3 kLocalVertices[3]{ { -size_, size_, 0 }, ... }` — 根拠: Player.h:41-45
- **`char keys[256]{};` のゼロ初期化** — シーンごとの入力バッファ — 根拠: Scene.h:66-67,93-94,130-131
- **空のコンストラクタ定義** — `Camera::Camera() {}` 等 — 根拠: Camera.cpp:3, Bullet.cpp:4, Player.cpp:5, Enemy.cpp:8
- **メンバ初期化子リスト付きコンストラクタ** — `ResultScene(int finalScore) : score(finalScore) {}` — 根拠: Scene.h:123
- **抽象基底クラス（純粋仮想関数 = 0）** — `virtual void update(SceneManager&) = 0; virtual void draw() = 0;` — 根拠: Scene.h:37-42
- **`virtual ~Scene() = default;`** — 仮想デストラクタのデフォルト化 — 根拠: Scene.h:39
- **override 指定子** — 各シーンの update / draw — 根拠: Scene.h:62-63,113-114,125-126
- **基底ポインタ経由の多態呼び出し** — `Scene* current` → `current->update(*this)` / `current->draw()` — 根拠: Scene.h:46, Scene.cpp:151-157
- **前方宣言** — `class SceneManager;` — 根拠: Scene.h:35
- **static const int クラス定数** — `BULLET_MAX = 50`, `MAX = 20` — 根拠: Player.h:29, Enemy.h:20
- **static メンバ変数（配列）とクラス外定義** — `static Enemy enemies_[MAX];` / `Enemy Enemy::enemies_[Enemy::MAX];` — 根拠: Enemy.h:44, Enemy.cpp:6
- **static メンバ関数によるマネージャ機能** — GetInactive / UpdateAll / CheckPlayerCollision / DrawAll / ResetAll — 根拠: Enemy.h:22-26, Enemy.cpp:47-51,95-108,112-154,167-172
- **同クラス別インスタンスの private 直接アクセス** — static 関数内で `e.isActive_`、`enemies_[i].position_` — 根拠: Enemy.cpp:103-106,118,125
- **関数内 static 変数** — 点滅タイマー `static int timer = 21;` — 根拠: Scene.cpp:175,239,348
- **クラス内インライン Getter / Setter** — Camera / Bullet / Player / Enemy — 根拠: Camera.h:10-19, Bullet.h:15-16, Player.h:24-31, Enemy.h:14-17
- **const 参照返しと const 値返し** — `const Vector3& GetPosition() const` / `const Vector3 GetRotation() const` — 根拠: Camera.h:10-11
- **const / 非 const アクセサの対** — `const Bullet& GetBullet(int) const` と `Bullet& GetBulletMutable(int)` — 根拠: Player.h:30-31
- **参照による入出力引数** — `int& playerScore`, `Player& player` — 根拠: Enemy.h:11,23-24, Scene.cpp:291-292
- **配列要素への参照エイリアス** — `Enemy& e = enemies_[j]; Bullet& b = player.GetBulletMutable(i);` — 根拠: Enemy.cpp:117,132,139
- **一時オブジェクト代入によるリセット** — `b = Bullet();` — 根拠: Enemy.cpp:148
- **if 条件内での宣言** — `if (Enemy* e = Enemy::GetInactive()) { ... }` — 根拠: Scene.cpp:285
- **nullptr の返却** — 空きスロット無し — 根拠: Enemy.cpp:171
- **new で生成し所有者が delete（所有権の移譲）** — `manager.setScene(new TitleScene())`、SceneManager が旧シーンを delete — 根拠: main.cpp:15, Scene.cpp:142-149,170,305,343
- **uint32_t** — ループ添字、色引数、定数 — 根拠: Player.cpp:69, Enemy.cpp:48,96, Scene.h:29, Scene.cpp:49,54,72,100
- **static_cast / C キャスト / 関数形式キャスト** — `static_cast<float>(rand())`, `(int)centerScreen.x`, `int(screenVertices[0].x)`, `float(M_PI)` — 根拠: Enemy.cpp:14,84; Player.cpp:76; Scene.cpp:36,50,55,101
- **k プレフィックスの const ローカル定数** — kGridHalfWidth, kSubdivision, kGridEvery, kLonEvery, kLatEvery — 根拠: Scene.cpp:48-50,100-102
- **グローバル const 定数** — kWindowTitle / kWindowWidth / kWindowHeight — 根拠: main.cpp:4-6
- **三項演算子** — 単位行列、軸線の色、入力方向 — 根拠: Matrix4x4.cpp:144, Scene.cpp:63,81, Player.cpp:10-11, Scene.cpp:224,230
- **const char* 引数** — `void Update(const char* keys)` — 根拠: Player.h:11
- **memcpy** — シーン内で前フレームキーを保存 — 根拠: Scene.cpp:166,213,338
- **printf 形式の可変引数** — `Novice::ScreenPrintf(x, y, "Score : %d", score)` — 根拠: Scene.cpp:330,354
- **ヘッダは Novice 非依存、cpp のみ <Novice.h>** — 根拠: Bullet.cpp:2, Player.cpp:3, Enemy.cpp:4, Scene.cpp:2, main.cpp:1
- **1 行複数文** — `hp_ -= damage; if (hp_ < 0) hp_ = 0;` — 根拠: Player.cpp:49
- **ドキュメント（ReadMe.md）** — 操作方法・加点要件・独自仕様を Markdown で記述 — 根拠: ReadMe.md:1-24
- **Git 運用ファイル** — Visual Studio 用 .gitignore（ビルド成果物除外）、.gitattributes（`* text=auto`） — 根拠: .gitignore:1-5,19-34, .gitattributes:4

### B. 数学・物理・当たり判定

- **Vector3 の線形補間 Lerp** — `a + (b−a)*t` を成分ごとに計算 — 根拠: Scene.cpp:5-11
- **EaseInOut（区分 2 次）** — t<0.5 で 2t²、以降 1−2(1−t)² — 根拠: Scene.cpp:13-20
- **経過時間ベースのアニメーション** — dt=1/60 を加算、t=time/duration を 1 でクランプ — 根拠: Scene.cpp:251-256
- **4x4 行列：乗算 / 転置 / 単位行列** — 二重ループでの積、`m[i][j] = m[j][i]`、対角 1 — 根拠: Matrix4x4.cpp:114-148
- **行ベクトル・行優先の規約（平行移動は m[3][0..2]、変換は v×M）** — Transform は `x*m[0][0] + y*m[1][0] + z*m[2][0] + m[3][0]` — 根拠: Matrix4x4.cpp:155-161,171-182
- **同次座標の w 除算（透視除算）** — Transform 内で x, y, z を w で割る — 根拠: Matrix4x4.cpp:176-180
- **平行移動行列 / 拡大縮小行列** — 単位行列を作ってから要素を設定 — 根拠: Matrix4x4.cpp:155-169
- **X / Y / Z 軸回転行列** — cos / sin の配置（RotX: m[1][2]=s, m[2][1]=−s など） — 根拠: Matrix4x4.cpp:185-224
- **アフィン行列 = S × (Rx × Ry × Rz) × T** — 根拠: Matrix4x4.cpp:227-242
- **透視投影行列（fovY, aspect, near, far、深度 0〜1）** — yScale=1/tan(fovY/2)、xScale=yScale/aspect、m[2][2]=f/(f−n)、m[2][3]=1、m[3][2]=−nf/(f−n) — 根拠: Matrix4x4.cpp:257-271
- **正射影行列（4x4）** — 2/(r−l) 等の対角と平行移動成分 — 根拠: Matrix4x4.cpp:274-287
- **ビューポート行列（Y 反転・深度範囲）** — m[0][0]=w/2、m[1][1]=−h/2、m[3][0]=left+w/2、m[3][1]=top+h/2 — 根拠: Matrix4x4.cpp:290-302
- **3x3 行列（2D）：正射影 / ビューポート / 拡縮 / 回転 / 平行移動 / 乗算 / 逆行列（行列式と余因子）/ 変換（w 除算）/ パイプライン合成** — 根拠: Matrix4x4.cpp:7-105（逆行列 67-90）
- **ビュー行列 = Rx·Ry·Rz·T(−pos)** — カメラ位置の符号反転と回転行列の合成 — 根拠: Camera.cpp:5-23
- **WVP 合成とスクリーン座標化** — world×view×proj → Transform（NDC）→ viewport で Transform — 根拠: Bullet.cpp:27-30, Player.cpp:55-58,65-72, Scene.cpp:29-33
- **原点 {0,0,0} を WVP で変換してオブジェクトの画面位置を得る** — 根拠: Bullet.cpp:29, Player.cpp:57, Enemy.cpp:163
- **ローカル頂点→ワールド→スクリーンで三角形描画** — kLocalVertices 3 点をアフィン×VP で変換し DrawTriangle — 根拠: Player.h:40-45, Player.cpp:65-78
- **VP と Viewport をまとめた 1 行列（vpv）で格子描画** — 根拠: Scene.cpp:52,60-61,78-79
- **XZ 平面グリッドの生成** — 半幅 256・10 分割、原点を通る軸線を赤 / 青で描く — 根拠: Scene.cpp:46-89
- **球のワイヤフレーム（緯度・経度のパラメトリック）** — `(r cosLat cosLon, r sinLat, r cosLat sinLon)` を 16 分割し隣接点と線で接続 — 根拠: Scene.cpp:95-136
- **球の見かけ半径（スクリーン半径）の算出** — ビュー空間で +X に半径分ずらした点を投影し中心との差の絶対値を取る — 根拠: Enemy.cpp:56-80
- **速度・加速度による落下と反発（反発係数 0.8）** — `vel.y += acc.y`、地面で位置補正＋`vel.y = −vel.y × restitution_` — 根拠: Enemy.cpp:18-19,29-38
- **円同士の当たり判定（sqrt 距離 < r1 + r2）** — 敵-プレイヤー、敵-弾（弾半径 10 固定） — 根拠: Enemy.cpp:120-124,142-146
- **入力ベクトルの正規化（3 成分）** — 根拠: Player.cpp:13-19
- **壁クランプ（サイズ考慮）** — x: ±580−size、y: size〜600−size — 根拠: Player.cpp:24-28
- **乱数：範囲・確率・float スケール** — `15 + rand()%41`、`rand()%58 == 0` でスポーン、`100 + rand()%501`、`rand()/RAND_MAX` — 根拠: Enemy.cpp:14,22; Scene.cpp:284-286
- **連続回転（毎フレーム角度加算）** — `rotate.x += 0.04f` — 根拠: Player.cpp:30
- **カメラのヨー回転と上下移動** — rot.y ± rotateSpeed、pos.y ± moveSpeed — 根拠: Scene.cpp:222-232
- **投影パラメータ** — fovY 0.50 rad、アスペクト 1280/720、near 0.1、far 2000、ビューポート 0,0,1280,720,深度 0〜2000 — 根拠: Scene.cpp:197-202
- **Z=0 平面上の 2D ゲームプレイを 3D 投影で表示** — 敵 / 弾 / プレイヤーの z は 0 — 根拠: Scene.cpp:287, Bullet.cpp:9-11, Player.h:34

### C. ゲームの構造・実装パターン

- **Scene 基底クラス＋SceneManager** — setScene(new X) で旧シーンを delete、update / draw を現在のシーンへ委譲 — 根拠: Scene.h:37-53, Scene.cpp:142-157
- **タイトル→ゲーム→リザルト→タイトルの遷移** — SPACE / HP 0 / ENTER をトリガーに setScene — 根拠: Scene.cpp:169-171,304-307,342-344
- **リザルトへスコアをコンストラクタ引数で渡す** — `new ResultScene(score)` — 根拠: Scene.cpp:305, Scene.h:123
- **シーンごとに入力バッファを持ち update 内で取得** — keys / preKeys をメンバ化 — 根拠: Scene.h:66-67,93-94,130-131, Scene.cpp:165-167
- **main はループと manager 呼び出しのみ** — `manager.update(); manager.draw();` — 根拠: main.cpp:14-15,34,44
- **ゲーム進行フラグ（cameraAnimating / playerActive / enemyActive / isPaused）** — 根拠: Scene.h:102-109, Scene.cpp:204-208
- **ポーズ機能** — P でトグル、ポーズ中は update を早期 return、WASD がカメラ操作に切替、R でカメラ初期化、"State : Pause" 点滅 — 根拠: Scene.cpp:216-248
- **開始演出（カメラの Lerp＋イージング移動）とスキップ** — 完了 / スキップでプレイヤー・敵を有効化 — 根拠: Scene.cpp:250-279
- **static メンバによる敵マネージャ（プール 20）** — 空き取得→Spawn、全更新・全描画・全リセット・衝突判定 — 根拠: Enemy.h:19-26, Enemy.cpp:47-51,95-108,112-154,167-172
- **タイトル遷移時に敵プールをリセット** — TitleScene コンストラクタで Enemy::ResetAll — 根拠: Scene.cpp:163
- **プレイヤー内包の弾プール（50 発、先頭の非アクティブを発射）** — 弾クラスをメンバ配列として持つ（ReadMe の「包含」） — 根拠: Player.h:47, Player.cpp:33-46, ReadMe.md:17-18
- **弾のライフサイクル** — Shoot で有効化（プレイヤー右端から）、右へ等速、x>640 で無効化 — 根拠: Bullet.cpp:6-22
- **敵のライフサイクル** — 確率スポーン（x=700, y 乱数）、左へ乱数速度、地面で跳ねる、壁到達で消滅＆スコア −100 — 根拠: Enemy.cpp:10-45, Scene.cpp:283-293
- **衝突結果の処理** — 敵-プレイヤー：敵消滅＋HP −10、弾-敵：両者消滅＋スコア +100、1 ヒットで break — 根拠: Enemy.cpp:112-154, Player.cpp:48-50
- **スコアを int& で共有** — GameScene のメンバを Enemy の static 関数に参照で渡す — 根拠: Enemy.h:11,23-24, Scene.cpp:291-292
- **HP バー（DrawBox の幅 = HP）とラベル** — 根拠: Player.cpp:87-89
- **点滅テキスト（関数内 static タイマーの周期）** — 根拠: Scene.cpp:174-184,239-246,347-358
- **ゲームオーバー条件** — HP ≤ 0 でリザルトへ、直後に return — 根拠: Scene.cpp:303-307
- **描画順序** — グリッド→地面線 / 壁線→球→敵→プレイヤー（弾・HP）→スコア — 根拠: Scene.cpp:312-331
- **3D デバッグ描画ヘルパー（DrawGrid / DrawWorldLine / DrawSphere）** — 根拠: Scene.h:13-29, Scene.cpp:24-136
- **行列の更新タイミング** — projection / viewport はコンストラクタで 1 回、view は毎フレーム — 根拠: Scene.cpp:195-202,281,314
- **Camera クラス（位置・回転・速度の Getter / Setter と GetViewMatrix）** — 根拠: Camera.h:5-29
- **状態表示（ScreenPrintf で "State : Game" / "State : Pause"）** — 根拠: Scene.cpp:245,309
- **ReadMe による仕様・操作説明と加点要件の記述** — 根拠: ReadMe.md:1-24

### D. 使用しているフレームワーク API（Novice）

- **Novice::Initialize / ProcessMessage / BeginFrame / EndFrame / Finalize** — 根拠: main.cpp:12,22,24,51,60
- **Novice::GetHitKeyStateAll** — main と各シーン update — 根拠: main.cpp:28, Scene.cpp:167,214,339
- **Novice::DrawTriangle(x1, y1, x2, y2, x3, y3, color, fillMode)** — プレイヤー — 根拠: Player.cpp:75-78
- **Novice::DrawEllipse** — 弾・敵 — 根拠: Bullet.cpp:40-41, Enemy.cpp:83-91
- **Novice::DrawLine** — ワールド線・格子・球 — 根拠: Scene.cpp:35-39,65-69,83-87,132-133
- **Novice::DrawBox** — HP バー — 根拠: Player.cpp:89
- **Novice::ScreenPrintf(x, y, fmt, ...)** — テキスト / スコア / 状態表示 — 根拠: Player.cpp:88, Scene.cpp:180,182,245,309,330,353-356
- **kFillModeSolid** — 根拠: Bullet.cpp:41, Player.cpp:77, Enemy.cpp:90
- **DIK_W / A / S / D / SPACE / P / R / RETURN / ESCAPE** — 根拠: Player.cpp:10-11; Scene.cpp:169,217,223,229,234,272,298,342; main.cpp:54

### E. 判断に迷うもの・備考

- **ReadMe と実装の不一致** — 演出スキップは「K」と記載だが実装は ENTER（DIK_RETURN） — 根拠: ReadMe.md:5, Scene.cpp:272
- **3x3 行列群は定義のみで未使用** — かつ平行移動が m[0][2] にある列ベクトル配置なのに MakePipeline は world×view×… の順で、4x4 側（行ベクトル）と規約が混在 — 根拠: Matrix4x4.cpp:45-50,92-105（使用箇所は grep で 0 件）
- **未使用の定義** — Transpose、4x4 MakeOrthographicMatrix、4x4 MakePipeline、Enemy::GetScreenPos、Enemy::gravity_（acc_.y を使用）、Camera::SetScale / SetRotateSpeed、Player::GetBullet(const)、Enemy::GetRotation — 根拠: Matrix4x4.cpp:129,245,274; Enemy.cpp:158-165; Enemy.h:40,16; Camera.h:18-19; Player.h:30
- **4x4 の逆行列は未実装（3x3 のみ）** — ビュー行列は逆行列ではなく回転×T(−pos) で構成 — 根拠: Matrix4x4.h:21, Camera.cpp:5-23
- **ビュー行列は厳密なカメラ逆変換ではない** — 回転の向きを反転せず、順序も R→T(−pos)（この作品の固定的なカメラ用途では見た目上成立している） — 根拠: Camera.cpp:19-22
- **Transform の w≠0 assert がコメントアウト** — 根拠: Matrix4x4.cpp:177
- **update 中に自分自身を delete する遷移** — TitleScene::update 内の setScene が current（自分）を delete。直後に return しているため動作している — 根拠: Scene.cpp:146-149,169-171,304-307
- **Camera::SetScale が moveSpeed_ を設定（命名不一致）** — 根拠: Camera.h:18
- **Vector3 を 2 要素で初期化** — `Vector3 vel_{ 0, 0 };`（z は値初期化で 0） — 根拠: Enemy.h:35-36
- **imgui.ini は存在するがコードに ImGui 呼び出しは無い** — grep で ImGui 0 件（PG2）
- **関数定義末尾の余分な `;`** — 根拠: Matrix4x4.cpp:126,137
- **1_game フォルダは exe のみ** — ソース無し
- **std::vector / スマートポインタは未使用** — 固定長配列・static 配列・生ポインタ（Scene*）で管理

### このソースで学んだと言える主要トピック

- 仮想関数による Scene 基底クラス＋SceneManager でのシーン遷移（タイトル / ゲーム / リザルト）、純粋仮想・override・`= default`
- 4x4 行列ライブラリの自作（行ベクトル規約：平行移動 / 拡縮 / 回転 X,Y,Z / アフィン / 透視投影 / 正射影 / ビューポート、透視除算）
- 3D→スクリーンの変換パイプライン（World×View×Projection → NDC → Viewport）とローカル頂点の三角形描画
- 3D デバッグ描画（XZ グリッド、ワールド線、球のワイヤフレーム）と球の見かけ半径の投影計算
- Lerp＋EaseInOut による時間ベースのカメラ演出、スキップ、ポーズ中のカメラ操作（ヨー回転・上下移動）
- static メンバによる敵マネージャ（プール 20）と、クラス内包の弾プール（包含）
- 速度・加速度・反発係数による簡易物理と円同士の当たり判定（sqrt 距離）
- NSDMI、クラス内インライン Getter / Setter、const / 非 const アクセサ、参照による入出力（int& score）、if 条件内宣言
- Novice の DrawTriangle / DrawEllipse / DrawLine / ScreenPrintf、ReadMe による提出物の説明、.gitignore / .gitattributes

---

## 4. 3 ソース横断の補足

### 4.1 3 ソースに共通して出てくるもの

- Novice のゲームループ骨格（Initialize / ProcessMessage / BeginFrame / EndFrame / Finalize）、WinMain、`const char kWindowTitle[]` — AL2_01 main.cpp:37-191, AL2_02 main.cpp:227-341, PG2 main.cpp:4-62
- keys / preKeys（char[256]）＋memcpy＋GetHitKeyStateAll、`keys[X] && !preKeys[X]` のトリガー判定、ESC 終了 — 3 ソースの main.cpp
- `struct Vector2 { float x, y; }` を各プロジェクトで再定義 — AL2_01 Function.h:3-6, AL2_02 main.cpp:9-12, PG2 Vector3.h:3-5
- static_cast、三項演算子、continue / 早期 return、`#pragma region`、`#pragma once`
- rand() による乱数（AL2_01・PG2）、反対キー同時押しの相殺（`a != b`）（AL2_01・AL2_02・PG2）
- 固定長配列＋フラグによるオブジェクトプール（AL2_01 のパーティクル、AL2_02 の敵、PG2 の敵・弾）
- 0xRRGGBBAA の色指定と kFillModeSolid

### 4.2 1 つのソースにしか出てこないもの

- AL2_01 のみ：new[] / delete[]、ダブルポインタ、std::min / std::max、double、protected、enum、ビット演算による色操作、イージング（EaseInBack）、矩形 / 円-矩形判定、マップチップ、カメラシェイク、kFillModeWireFrame、色定数 RED
- AL2_02 のみ：std::vector、std::string / to_string、範囲 for、_countof、条件コンパイル（#ifdef）、ImGui、XML ドキュメントコメント、関数ベース（非クラス）の構成
- PG2 のみ：純粋仮想関数 / override / `= default`、static メンバ変数・static メンバ関数、関数内 static、class メンバへの NSDMI の広範な使用（struct の既定値は AL2_01 / AL2_02 にもある）、関数オーバーロード、行列（3x3 / 4x4）、透視投影、Lerp / EaseInOut、反発係数、ScreenPrintf、DrawTriangle、ReadMe / .gitignore
- assert は AL2_02 と PG2、virtual / 継承は AL2_01 と PG2、LoadTexture / DrawSprite は AL2_01 と AL2_02 のみ（PG2 はテクスチャ無し）

### 4.3 3 ソースのどこにも無いもの（grep で 0 件を確認）

- 言語機能：template、auto、ラムダ式、enum class（enum は非スコープのみ）、constexpr、名前空間の定義（namespace）、operator オーバーロード（ベクトル演算は成分ごとの手書きか自由関数）、switch / case、do-while、friend、typedef / using、size_t、inline キーワード、explicit、mutable、noexcept、static_assert、reinterpret_cast / dynamic_cast / const_cast、NULL（nullptr は使用）、struct の継承
- 標準ライブラリ：std::unique_ptr / shared_ptr / make_unique、std::function、std::array、std::list、<fstream> / <sstream>（CSV などのファイル読込は無く、マップ・ステージ表はソースに直書き）、<random> / srand、std::clamp、std::swap、floor / ceil / pow / atan 系
- Novice API：音声（LoadAudio / PlayAudio）、マウス（GetMouse 系）、ゲームパッド（GetAnalog / IsPressButton 系）、DrawSpriteRect、DrawQuad、SetBlendMode、IsTriggerKey / IsPressKey（キー判定は配列方式のみ）、ConsolePrintf
- ゲーム要素：画面のフェードイン / アウト（AL2_01 の残像アルファフェードは個別オブジェクトのみ）、CSV / 外部ファイル読込、サウンド、スプライトアニメーション（コマ送り）、デルタタイム（PG2 は 1/60 固定加算）
