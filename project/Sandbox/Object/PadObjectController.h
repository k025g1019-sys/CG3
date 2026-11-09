#pragma once

#include <string>
#include <vector>

#include "Engine/Rendering/Object3D.h"

/// <summary>
/// XBoxコントローラーで「選択中のオブジェクト」を操作するコントローラ。
/// 毎フレームSetTargetsで操作対象リストを渡し、Updateでパッド入力をTransformへ反映する。
///   左スティック : XZ平面の平行移動（ワールド軸基準）
///   右スティック : 回転（横=Y軸ヨー / 縦=X軸ピッチ）
///   RT / LT      : 拡大 / 縮小（踏み込み量に応じた倍率）
///   B / A        : Y軸の上昇 / 下降
///   RB / LB      : 選択オブジェクトを次へ / 前へ切り替え
/// </summary>
class PadObjectController {
public:
    // 操作対象1件（objectの所有権は持たない）
    struct Target {
        std::string label;         // ImGuiに表示する名前
        Engine::Object3D* object;  // 操作対象
    };

    // 操作対象リストを差し替える（毎フレーム呼ぶ。選択インデックスは範囲内へ丸められる）
    void SetTargets(std::vector<Target> targets);

    // パッド入力を選択中オブジェクトのTransformへ反映する（Input::Updateの後に毎フレーム呼ぶ）
    void Update();

    // 選択中のオブジェクトを返す（対象が無ければnullptr。軸ギズモの追従対象などに使う）
    Engine::Object3D* GetSelectedObject() const {
        return targets_.empty() ? nullptr : targets_[size_t(selectedIndex_)].object;
    }

#ifdef USE_IMGUI
    // パッドの接続状態・選択中オブジェクトのCombo・操作説明を描画する
    void DrawImGui();
#endif

private:
    std::vector<Target> targets_;
    int selectedIndex_ = 0;  // 選択中オブジェクト（targets_のインデックス）
};
