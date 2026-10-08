#include "Game/Stage/CameraController.h"

#include "Engine/Math/Matrix4x4.h"
#include "Game/Object/Player.h"
#include "Game/Stage/Stage.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

#pragma region 初期化

void CameraController::Initialize(Camera* camera, const Player* target, const Stage* stage) {
	assert(camera);
	assert(target);
	assert(stage);
	camera_ = camera;
	target_ = target;
	stage_ = stage;

	// 横視点：回転なしで +Z を向く
	camera_->GetTransform().rotate = { 0.0f, 0.0f, 0.0f };
}

#pragma endregion

#pragma region 更新

void CameraController::Update(float aspectRatio) {
	Transform3D& transform = camera_->GetTransform();

	// X だけ補間で追従し、ステージの端で止める
	const float halfViewWidth = CalcHalfViewWidth(aspectRatio);
	float x = Lerp(transform.translate.x, CalcTargetX(), kFollowRate);
	x = ClampX(x, halfViewWidth);

	transform.translate = { x, kHeight, -kDistance };
	transform.rotate = { 0.0f, 0.0f, 0.0f };
}

void CameraController::Reset(float aspectRatio) {
	Transform3D& transform = camera_->GetTransform();

	const float halfViewWidth = CalcHalfViewWidth(aspectRatio);
	transform.translate = { ClampX(CalcTargetX(), halfViewWidth), kHeight, -kDistance };
	transform.rotate = { 0.0f, 0.0f, 0.0f };
}

float CameraController::CalcHalfViewWidth(float aspectRatio) const {
	// 縦の画角の半分の tan × 距離 ＝ 画面に入る高さの半分。横はそのアスペクト比倍
	const float halfViewHeight = std::tan(camera_->GetFovY() * 0.5f) * kDistance;
	return halfViewHeight * aspectRatio;
}

float CameraController::ClampX(float x, float halfViewWidth) const {
	const float minX = stage_->GetLeftEdge() + halfViewWidth;
	const float maxX = stage_->GetRightEdge() - halfViewWidth;

	// ステージが画面より狭いときは中央に固定する
	if (minX >= maxX) {
		return (stage_->GetLeftEdge() + stage_->GetRightEdge()) * 0.5f;
	}
	return std::clamp(x, minX, maxX);
}

float CameraController::CalcTargetX() const {
	return target_->GetPosition().x + kOffsetX;
}

#pragma endregion

#pragma region 描画

#ifdef USE_IMGUI
void CameraController::DrawImGui() {
	if (ImGui::TreeNode("Camera")) {
		ImGui::DragFloat("distance", &kDistance, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("height", &kHeight, 0.1f);
		ImGui::DragFloat("followRate", &kFollowRate, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("offsetX", &kOffsetX, 0.1f);
		const Vector3& position = camera_->GetTransform().translate;
		ImGui::Text("position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
