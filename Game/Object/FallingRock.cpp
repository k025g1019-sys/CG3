#include "Game/Object/FallingRock.h"

#include "Game/Stage/Stage.h"
#include "Game/Util/GlobalVariables.h"

#include <cassert>

using namespace Engine;

void FallingRock::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "FallingRock";
	globalVariables->CreateGroup(groupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(groupName, "Gravity", kGravity);
	globalVariables->AddItem(groupName, "MaxFallSpeed", kMaxFallSpeed);
	globalVariables->AddItem(groupName, "Radius", kRadius);
	globalVariables->AddItem(groupName, "SpinSpeed", kSpinSpeed);
}

void FallingRock::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "FallingRock";
	kGravity = globalVariables->GetFloatValue(groupName, "Gravity");
	kMaxFallSpeed = globalVariables->GetFloatValue(groupName, "MaxFallSpeed");
	kRadius = globalVariables->GetFloatValue(groupName, "Radius");
	kSpinSpeed = globalVariables->GetFloatValue(groupName, "SpinSpeed");
}

void FallingRock::Initialize(const Vector3& position, const Stage* stage) {
	assert(stage);
	stage_ = stage;
	position_ = { position.x, position.y, 0.0f };
	fallSpeed_ = 0.0f;

	model_.Initialize(Primitive::kSphere);
	model_.SetColor({ 0.5f, 0.42f, 0.35f, 1.0f });
	model_.GetTransform().scale = { kRadius * 2.0f, kRadius * 2.0f, kRadius * 2.0f };
	model_.GetTransform().translate = position_;
	model_.Update();
}

void FallingRock::Update() {
	// 重力で加速しながら落ちる
	fallSpeed_ += kGravity;
	if (fallSpeed_ > kMaxFallSpeed) {
		fallSpeed_ = kMaxFallSpeed;
	}
	position_.y -= fallSpeed_;

	// 床に着いたら砕けて消える
	if (position_.y - kRadius <= stage_->GetGroundY()) {
		Kill();
	}

	Transform3D& transform = model_.GetTransform();
	transform.translate = position_;
	transform.rotate.z += kSpinSpeed;
	model_.Update();
}

void FallingRock::UpdateModel() {
	model_.Update();
}

void FallingRock::Draw() const {
	model_.Draw();
}
