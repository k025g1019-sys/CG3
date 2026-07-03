#pragma once

#include <memory>

#include "Game/Scene/BaseScene.h"

// アプリ内のシーンの識別子
enum class SceneId {
    kStereoDemo,  // 立体視デモ（テクスチャ付き立方体を奥行き違いに並べたシーン）
    kGame,        // 従来のデモシーン（三角形・球・OBJ・スプライト）
};

// 起動直後に表示するシーン（ここを変えるだけで最初のシーンを差し替えられる）
inline constexpr SceneId kInitialSceneId = SceneId::kStereoDemo;

// 指定IDのシーンを生成する（Initializeは呼び出し側で行う）
std::unique_ptr<BaseScene> CreateScene(SceneId id);
