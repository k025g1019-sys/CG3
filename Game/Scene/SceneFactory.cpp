#include "Game/Scene/SceneFactory.h"

#include "Game/Scene/GameScene.h"
#include "Game/Scene/StereoDemoScene.h"

std::unique_ptr<BaseScene> CreateScene(SceneId id) {
	switch (id) {
	case SceneId::kStereoDemo:
		return std::make_unique<StereoDemoScene>();
	case SceneId::kGame:
		return std::make_unique<GameScene>();
	}
	return nullptr;
}
