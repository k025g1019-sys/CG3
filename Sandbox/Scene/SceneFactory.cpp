#include "Sandbox/Scene/SceneFactory.h"

#include "Sandbox/Scene/AxisScene.h"
#include "Sandbox/Scene/GameScene.h"
#include "Sandbox/Scene/PlaneScene.h"
#include "Sandbox/Scene/StereoDemoScene.h"

std::unique_ptr<Engine::BaseScene> CreateScene(SceneId id) {
	switch (id) {
	case SceneId::kStereoDemo:
		return std::make_unique<StereoDemoScene>();
	case SceneId::kPlane:
		return std::make_unique<PlaneScene>();
	case SceneId::kGame:
		return std::make_unique<GameScene>();
	case SceneId::kAxis:
		return std::make_unique<AxisScene>();
	}
	return nullptr;
}

SceneId NextDemoSceneId(SceneId id) {
	switch (id) {
	case SceneId::kPlane:
		return SceneId::kGame;
	case SceneId::kGame:
		return SceneId::kAxis;
	case SceneId::kAxis:
		return SceneId::kPlane;
	case SceneId::kStereoDemo:
		// 立体視デモは通常デモシーンの巡回に入らない（Tabでは切り替えない）
		return SceneId::kStereoDemo;
	}
	return id;
}
