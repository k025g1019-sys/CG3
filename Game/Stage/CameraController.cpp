#include "Game/Stage/CameraController.h"

#include "Engine/Math/Matrix4x4.h"
#include "Game/Object/Player.h"
#include "Game/Stage/Stage.h"
#include "Game/Util/GlobalVariables.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

#pragma region 調整値

void CameraController::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "CameraController";
	globalVariables->CreateGroup(groupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(groupName, "Distance", kDistance);
	globalVariables->AddItem(groupName, "Height", kHeight);
	globalVariables->AddItem(groupName, "FollowRate", kFollowRate);
	globalVariables->AddItem(groupName, "OffsetX", kOffsetX);
	globalVariables->AddItem(groupName, "FocusRate", kFocusRate);
	globalVariables->AddItem(groupName, "MaxFocusOffset", kMaxFocusOffset);
}

void CameraController::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "CameraController";
	kDistance = globalVariables->GetFloatValue(groupName, "Distance");
	kHeight = globalVariables->GetFloatValue(groupName, "Height");
	kFollowRate = globalVariables->GetFloatValue(groupName, "FollowRate");
	kOffsetX = globalVariables->GetFloatValue(groupName, "OffsetX");
	kFocusRate = globalVariables->GetFloatValue(groupName, "FocusRate");
	kMaxFocusOffset = globalVariables->GetFloatValue(groupName, "MaxFocusOffset");
}

#pragma endregion

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

void CameraController::SetFocusPosition(const Vector3& position) {
	hasFocus_ = true;
	focusPosition_ = position;
}

float CameraController::CalcTargetX() const {
	float x = target_->GetPosition().x + kOffsetX;

	// 注目点があれば、その方へ割合ぶん寄せる（寄せすぎて対象が画面から外れないよう上限あり）
	if (hasFocus_) {
		const float toFocus = (focusPosition_.x - target_->GetPosition().x) * kFocusRate;
		x += std::clamp(toFocus, -kMaxFocusOffset, kMaxFocusOffset);
	}
	return x;
}

#pragma endregion

#pragma region 描画

#ifdef USE_IMGUI
void CameraController::DrawImGui() {
	if (ImGui::TreeNode("Camera")) {
		const Vector3& position = camera_->GetTransform().translate;
		ImGui::Text("position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
