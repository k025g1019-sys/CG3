#include "Game/Stage/Stage.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

#pragma region 初期化

void Stage::Initialize() {
	// 仮モデル（組み込みの立方体）。床は緑がかった色、壁は灰色で見分ける
	floor_.Initialize(Primitive::kCube);
	floor_.SetColor({ 0.35f, 0.55f, 0.35f, 1.0f });

	leftWall_.Initialize(Primitive::kCube);
	leftWall_.SetColor({ 0.45f, 0.45f, 0.5f, 1.0f });

	rightWall_.Initialize(Primitive::kCube);
	rightWall_.SetColor({ 0.45f, 0.45f, 0.5f, 1.0f });

	ApplyLayout();
}

#pragma endregion

#pragma region 更新

void Stage::Update() {
	// ImGui で幅を変えたときも追従するよう、毎フレーム配置し直す
	ApplyLayout();

	floor_.Update();
	leftWall_.Update();
	rightWall_.Update();
}

void Stage::ApplyLayout() {
	// 床：左右の壁の外側まで届く長さにし、上面が kGroundY に来るよう下へ下げる
	Transform3D& floor = floor_.GetTransform();
	floor.scale = { kWidth + kWallThickness * 2.0f, kFloorThickness, kFloorDepth };
	floor.translate = { 0.0f, kGroundY - kFloorThickness * 0.5f, 0.0f };

	// 壁：端のすぐ外側に立てる
	Transform3D& left = leftWall_.GetTransform();
	left.scale = { kWallThickness, kWallHeight, kFloorDepth };
	left.translate = { GetLeftEdge() - kWallThickness * 0.5f, kGroundY + kWallHeight * 0.5f, 0.0f };

	Transform3D& right = rightWall_.GetTransform();
	right.scale = { kWallThickness, kWallHeight, kFloorDepth };
	right.translate = { GetRightEdge() + kWallThickness * 0.5f, kGroundY + kWallHeight * 0.5f, 0.0f };
}

#pragma endregion

#pragma region 描画

void Stage::Draw() const {
	floor_.Draw();
	leftWall_.Draw();
	rightWall_.Draw();
}

#ifdef USE_IMGUI
void Stage::DrawImGui() {
	if (ImGui::TreeNode("Stage")) {
		ImGui::DragFloat("width", &kWidth, 0.1f, 4.0f, 200.0f);
		ImGui::Text("left: %.2f  right: %.2f  groundY: %.2f", GetLeftEdge(), GetRightEdge(), GetGroundY());
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
