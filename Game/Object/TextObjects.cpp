#include "Game/Object/TextObjects.h"

#include "Game/Util/GlobalVariables.h"

#include <cmath>
#include <numbers>

using namespace Engine;

#pragma region 初期化

void TextObjects::Initialize(const std::string& modelPath, const std::string& groupName,
	const Vector3& position, float scale, float bobAmplitude) {
	// モデル。陰影あり
	object_.Initialize(modelPath, LightingMode::kHalfLambert);

	// 既定値
	position_ = position;
	scale_ = scale;
	bobAmplitude_ = bobAmplitude;
	bobTimer_ = 0.0f;

	// 調整項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	groupName_ = groupName;
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	globalVariables->CreateGroup(groupName_);
	globalVariables->AddItem(groupName_, "Position", position_);
	globalVariables->AddItem(groupName_, "Scale", scale_);
	globalVariables->AddItem(groupName_, "Color", color_);
	globalVariables->AddItem(groupName_, "BobAmplitude", bobAmplitude_);
	globalVariables->AddItem(groupName_, "BobPeriod", bobPeriod_);
}

void TextObjects::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	position_ = globalVariables->GetVector3Value(groupName_, "Position");
	scale_ = globalVariables->GetFloatValue(groupName_, "Scale");
	color_ = globalVariables->GetVector3Value(groupName_, "Color");
	bobAmplitude_ = globalVariables->GetFloatValue(groupName_, "BobAmplitude");
	bobPeriod_ = globalVariables->GetFloatValue(groupName_, "BobPeriod");
}

#pragma endregion

#pragma region 更新

void TextObjects::Update() {
	// ImGui での変更を毎フレーム反映する
	ApplyGlobalVariables();

	// 揺れの時間を進める（1 往復ぶん進んだら 0 に戻す）
	bobTimer_ += kFrameTime;
	if (bobPeriod_ > 0.0f && bobTimer_ >= bobPeriod_) {
		bobTimer_ -= bobPeriod_;
	}

	// sin で上下にゆっくり揺らす（1 往復 = bobPeriod_ 秒）
	float offsetY = 0.0f;
	if (bobPeriod_ > 0.0f) {
		const float angle = 2.0f * std::numbers::pi_v<float> * bobTimer_ / bobPeriod_;
		offsetY = std::sin(angle) * bobAmplitude_;
	}

	Transform3D& transform = object_.GetTransform();
	transform.scale = { scale_, scale_, scale_ };
	transform.translate = { origin_.x + position_.x, origin_.y + position_.y + offsetY, origin_.z + position_.z };
	object_.SetColor({ color_.x * brightness_, color_.y * brightness_, color_.z * brightness_, 1.0f });
	object_.Update();
}

#pragma endregion

#pragma region 描画

void TextObjects::Draw() const {
	object_.Draw();
}

#pragma endregion
