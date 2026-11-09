#pragma once

#include <memory>

#include "Engine/Scene/BaseScene.h"

// アプリ内のシーンの識別子
enum class SceneId {
    kStereoDemo,  // 立体視デモ（テクスチャ付き立方体を奥行き違いに並べたシーン）
    kPlane,       // 最初のデモシーン（Instancingで板ポリ10枚を描く、パーティクルの土台）
    kGame,        // 2つ目のデモシーン（パーティクル。エミッタ・加速度場）
    kAxis,        // 3つ目のデモシーン（axis.obj・球・OBJモデルと2Dスプライト。鏡面反射の調整）
};

// 起動直後に表示するシーン（ここを変えるだけで最初のシーンを差し替えられる）
inline constexpr SceneId kInitialSceneId = SceneId::kPlane;

// Tabキーで切り替える次の通常デモシーン（板ポリ → パーティクル → 鏡面反射 → 板ポリ の順に回る）
SceneId NextDemoSceneId(SceneId id);

// 指定IDのシーンを生成する（SceneManager::ChangeSceneに渡すと次のフレームで初期化される）
std::unique_ptr<Engine::BaseScene> CreateScene(SceneId id);
