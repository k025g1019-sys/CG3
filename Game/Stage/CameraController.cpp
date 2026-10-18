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
	globalVariables->AddItem(groupName, "FocusMargin", kFocusMargin);
	globalVariables->AddItem(groupName, "TargetMargin", kTargetMargin);
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
	kFocusMargin = globalVariables->GetFloatValue(groupName, "FocusMargin");
	kTargetMargin = globalVariables->GetFloatValue(groupName, "TargetMargin");
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

	// X だけ補間で追従する。対象が画面の端に寄りすぎないようにしてから、ステージの端で止める
	// （補間は揺れを足す前の位置から行う。揺れた位置から補間すると、揺れが追従の位置に残ってしまう）
	const float halfViewWidth = CalcHalfViewWidth(aspectRatio);
	float x = Lerp(currentX_, CalcTargetX(halfViewWidth), kFollowRate);
	x = ClampToKeepTarget(x, halfViewWidth);
	x = ClampX(x, halfViewWidth);
	currentX_ = x;

	// 揺れは見た目のカメラの位置にだけ足す
	const Vector3 shake = UpdateShake();
	transform.translate = { currentX_ + shake.x, kHeight + shake.y, -kDistance };
	transform.rotate = { 0.0f, 0.0f, 0.0f };
}

void CameraController::StartShake(float amplitude, float duration) {
	shakeAmplitude_ = amplitude;
	shakeDuration_ = duration;
	shakeTimer_ = duration;
}

Vector3 CameraController::UpdateShake() {
	if (shakeTimer_ <= 0.0f || shakeDuration_ <= 0.0f) {
		return { 0.0f, 0.0f, 0.0f };
	}
	shakeTimer_ -= kFrameTime;
	if (shakeTimer_ < 0.0f) {
		shakeTimer_ = 0.0f;
	}

	// 縦と横を少し違う速さで振り、残り時間の割合で小さくしていく
	const float elapsed = shakeDuration_ - shakeTimer_;
	const float rate = shakeTimer_ / shakeDuration_;
	return { std::sin(elapsed * kShakeSpeedX) * shakeAmplitude_ * rate, std::cos(elapsed * kShakeSpeedY) * shakeAmplitude_ * rate, 0.0f };
}

void CameraController::Reset(float aspectRatio) {
	Transform3D& transform = camera_->GetTransform();

	const float halfViewWidth = CalcHalfViewWidth(aspectRatio);
	const float x = ClampToKeepTarget(CalcTargetX(halfViewWidth), halfViewWidth);
	currentX_ = ClampX(x, halfViewWidth);
	shakeTimer_ = 0.0f;
	transform.translate = { currentX_, kHeight, -kDistance };
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

float CameraController::ClampToKeepTarget(float x, float halfViewWidth) const {
	// 対象が画面の中央から離れてよい距離（画面の半分の幅から、端に空けておく幅を引く）
	const float limit = halfViewWidth - kTargetMargin;
	if (limit <= 0.0f) {
		return target_->GetPosition().x;
	}
	const float targetX = target_->GetPosition().x;
	return std::clamp(x, targetX - limit, targetX + limit);
}

float CameraController::CalcTargetX(float halfViewWidth) const {
	float x = target_->GetPosition().x + kOffsetX;

	if (hasFocus_) {
		// 注目点の方へ割合ぶん寄せる（近いときの寄せ方。上限あり）
		const float toFocus = focusPosition_.x - target_->GetPosition().x;
		x += std::clamp(toFocus * kFocusRate, -kMaxFocusOffset, kMaxFocusOffset);

		// 対象が注目点から離れていくとき（と止まっているとき）は、注目点が画面の外（端から FocusMargin より外側）へ出ないよう、
		// 画面に入るところまでさらに寄せる。寄せすぎて対象が画面の端に寄りすぎる分は、あとで ClampToKeepTarget が止める（対象を映す方を優先）。
		// 注目点へ向かって動いている間は、この補正をしない（近づけば自然に画面へ入ってくる。補正すると、近づく途中でカメラが止まって見える）
		const float limit = halfViewWidth - kFocusMargin;
		if (limit > 0.0f && !IsTargetApproachingFocus()) {
			x = std::clamp(x, focusPosition_.x - limit, focusPosition_.x + limit);
		}
	}
	return x;
}

bool CameraController::IsTargetApproachingFocus() const {
	if (!hasFocus_) {
		return false;
	}

	// 対象の速度のうち、注目点へ向かう向きの大きさ（離れる向きなら負）
	const float velocityX = target_->GetVelocity().x;
	const bool isFocusOnRight = focusPosition_.x >= target_->GetPosition().x;
	const float approachSpeed = isFocusOnRight ? velocityX : -velocityX;
	return approachSpeed > kApproachSpeedThreshold;
}

#pragma endregion

#pragma region 描画

#ifdef USE_IMGUI
void CameraController::DrawImGui() {
	if (ImGui::TreeNode("Camera")) {
		const Vector3& position = camera_->GetTransform().translate;
		ImGui::Text("position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
		ImGui::Text("keep focus in view: %s", IsTargetApproachingFocus() ? "off (approaching)" : "on");
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
