#include "Game/Object/Bit.h"

#include "Game/Util/GlobalVariables.h"

#include <cmath>

using namespace Engine;

void Bit::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Bit";
	globalVariables->CreateGroup(groupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(groupName, "OrbitRadius", kOrbitRadius);
	globalVariables->AddItem(groupName, "AngularSpeed", kAngularSpeed);
	globalVariables->AddItem(groupName, "Radius", kRadius);
}

void Bit::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Bit";
	kOrbitRadius = globalVariables->GetFloatValue(groupName, "OrbitRadius");
	kAngularSpeed = globalVariables->GetFloatValue(groupName, "AngularSpeed");
	kRadius = globalVariables->GetFloatValue(groupName, "Radius");
}

void Bit::Initialize(const Vector3& anchor, float startAngle) {
	anchor_ = anchor;
	angle_ = startAngle;
	position_ = { anchor_.x + std::cos(angle_) * kOrbitRadius, anchor_.y + std::sin(angle_) * kOrbitRadius, 0.0f };

	model_.Initialize(Primitive::kSphere);
	model_.SetColor({ 0.7f, 0.4f, 0.9f, 1.0f });
	model_.GetTransform().scale = { kRadius * 2.0f, kRadius * 2.0f, kRadius * 2.0f };
	model_.GetTransform().translate = position_;
	model_.Update();
}

void Bit::Update() {
	// 中心の周りを等速で回る（三角関数による円運動）
	angle_ += kAngularSpeed;
	position_ = { anchor_.x + std::cos(angle_) * kOrbitRadius, anchor_.y + std::sin(angle_) * kOrbitRadius, 0.0f };

	model_.GetTransform().translate = position_;
	model_.Update();
}

void Bit::UpdateModel() {
	model_.Update();
}

void Bit::Draw() const {
	model_.Draw();
}
