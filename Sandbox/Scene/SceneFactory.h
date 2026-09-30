#pragma once

#include <memory>

#include "Engine/Scene/BaseScene.h"

// アプリ内のシーンの識別子
enum class SceneId {
    kStereoDemo,  // 立体視デモ（テクスチャ付き立方体を奥行き違いに並べたシーン）
    kGame,        // 最初のデモシーン（plane.obj・2Dスプライト・三角形2枚）
    kAxis,        // 2つ目のデモシーン（axis.obj・球）
};

// 起動直後に表示するシーン（ここを変えるだけで最初のシーンを差し替えられる）
inline constexpr SceneId kInitialSceneId = SceneId::kGame;

// 指定IDのシーンを生成する（SceneManager::ChangeSceneに渡すと次のフレームで初期化される）
std::unique_ptr<Engine::BaseScene> CreateScene(SceneId id);
