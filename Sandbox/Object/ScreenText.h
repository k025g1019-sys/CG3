#pragma once

#include <string>

#include "Engine/Math/Matrix4x4.h"
#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Rendering/Object3D.h"

/// <summary>
/// 3Dテキストモデル（Blenderで作った文字のOBJ）を、画面の左上に表示するオブジェクト。
/// 毎フレーム、描画に使うカメラの前（カメラから distance_ の距離）の左上へ置き直すため、
/// カメラ（デバッグカメラを含む）を動かしても画面上の位置と大きさは変わらない。
/// OBJは原点が文字の左上になるように作っておく（Blenderのテキストの配置を 左揃え・上揃え にする）。
/// 深度テストをしないPSO（kOverlay）で描くため、ほかの物体に隠れず常に最前面に表示される。
/// 文字は2つのOBJを重ねて描く：
///   本体（厚みのある文字。側面の色で描く）→ 表面（厚みなしの文字。文字の色で描く）
/// 表面を後から描くので文字の形は潰れない。文字は画面と平行に置き（行が斜めにならない）、
/// 本体だけを奥ほど斜めにずらして（シアー）側面をのぞかせ、立体感を出す。
///   Initialize : OBJを読み込む（ライティングなしで描く）。lineIndexで何行目に置くかを決める
///   Update     : カメラのビュー行列・画角・アスペクト比から置く位置と向きを決め、定数バッファへ書き込む
///                （毎フレーム、描画に使うビュー行列が決まった後に呼ぶ）
///   Draw       : 描画する（シーンのOnDrawの中で、ほかの物体の後に呼ぶ）
/// </summary>
class ScreenText {
public:
    /// <summary>
    /// OBJファイルで初期化する
    /// </summary>
    /// <param name="bodyObjFilePath">厚みのある文字（例: "resources/textObjects/tabSceneChange.obj"）</param>
    /// <param name="faceObjFilePath">同じ文字の、厚みなしの表面だけ（例: "resources/textObjects/tabSceneChangeFace.obj"）</param>
    /// <param name="lineIndex">左上から数えて何行目に置くか（0が一番上。1行ぶんの高さは文字の大きさ × kLineSpacing）</param>
    void Initialize(const std::string& bodyObjFilePath, const std::string& faceObjFilePath, int lineIndex = 0);

    /// <summary>
    /// 画面の左上に来るように置き直し、定数バッファへ書き込む（毎フレーム、Drawより前に呼ぶ）
    /// </summary>
    /// <param name="viewMatrix">描画に使うカメラのビュー行列</param>
    /// <param name="fovY">カメラの垂直視野角（ラジアン）</param>
    /// <param name="aspectRatio">描画先のアスペクト比（幅 / 高さ）</param>
    void Update(const Engine::Matrix4x4& viewMatrix, float fovY, float aspectRatio);

    // 描画する（シーンのOnDrawの中で呼ぶ）。深度テストをしないPSOで描き、描いた後は標準のPSOへ戻す
    void Draw() const;

    // カメラから文字までの距離（画面上の大きさは距離によらず同じ。立体視では、収束距離に合わせると飛び出さずに見える）
    void SetDistance(float distance) { distance_ = distance; }

#ifdef USE_IMGUI
    // 表示の調整UI（呼び出し側のウィンドウ内へ差し込む。Begin/Endは行わない）
    void DrawImGui(const char* label);
#endif

private:
    // 1行ぶんの高さ（Blenderの文字サイズ1.0に対する倍率。行どうしが少し離れるよう1より大きくする）
    static constexpr float kLineSpacing = 1.3f;

    // 厚みのある文字（側面の色で描く。奥ほど斜めにずらして、表面のまわりに側面がのぞくようにする）
    Engine::Object3D body_;

    // 厚みなしの文字の表面（本体の上に文字の色で描く）
    Engine::Object3D face_;

    // 左上から数えて何行目か（0が一番上）
    int lineIndex_ = 0;

    // カメラから文字までの距離（ワールド単位）
    float distance_ = 1.0f;

    // 文字の大きさ（Blenderの文字サイズ1.0ぶんが、画面の高さの何割になるか）
    float heightRatio_ = 0.05f;

    // 画面の左端・上端からの余白（画面の高さに対する割合）
    float marginRatio_ = 0.03f;

    // 厚み（側面）をのぞかせる向きと量（奥行き1あたり、x・yをどれだけずらすか。+xが右、+yが上）。
    // 0なら側面は見えない。大きくするほど側面が太く見えて立体的になる（既定は左下へのぞく向き）
    Engine::Vector2 depthDirection_{ -0.2f, -0.2f };

    // 文字の色（ライティングしないので、この色がそのまま出る）
    Engine::Vector4 color_{ 1.0f, 1.0f, 1.0f, 1.0f };

    // 側面の色（文字の色より暗くして、厚みが分かるようにする）
    Engine::Vector4 sideColor_{ 0.15f, 0.2f, 0.35f, 1.0f };
};
