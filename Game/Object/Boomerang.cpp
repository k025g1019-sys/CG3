#include "Game/Object/Boomerang.h"

#include "Engine/Math/Curve.h"
#include "Engine/Math/Matrix4x4.h"
#include "Game/Util/Easing.h"
#include "Game/Util/GlobalVariables.h"

#include <algorithm>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

#pragma region 調整値

void Boomerang::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Boomerang";
	globalVariables->CreateGroup(groupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(groupName, "Damage", kDamage);
	globalVariables->AddItem(groupName, "OutwardDuration", kOutwardDuration);
	globalVariables->AddItem(groupName, "ReturnStartSpeed", kReturnStartSpeed);
	globalVariables->AddItem(groupName, "ReturnAcceleration", kReturnAcceleration);
	globalVariables->AddItem(groupName, "ReturnMaxSpeed", kReturnMaxSpeed);
	globalVariables->AddItem(groupName, "CatchDistance", kCatchDistance);
	globalVariables->AddItem(groupName, "Radius", kRadius);
	globalVariables->AddItem(groupName, "SpinSpeed", kSpinSpeed);
	globalVariables->AddItem(groupName, "CurvePeakRate", kCurvePeakRate);
	globalVariables->AddItem(groupName, "DownCurveVerticalRate", kDownCurveVerticalRate);
}

void Boomerang::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Boomerang";
	kDamage = globalVariables->GetIntValue(groupName, "Damage");
	kOutwardDuration = globalVariables->GetFloatValue(groupName, "OutwardDuration");
	kReturnStartSpeed = globalVariables->GetFloatValue(groupName, "ReturnStartSpeed");
	kReturnAcceleration = globalVariables->GetFloatValue(groupName, "ReturnAcceleration");
	kReturnMaxSpeed = globalVariables->GetFloatValue(groupName, "ReturnMaxSpeed");
	kCatchDistance = globalVariables->GetFloatValue(groupName, "CatchDistance");
	kRadius = globalVariables->GetFloatValue(groupName, "Radius");
	kSpinSpeed = globalVariables->GetFloatValue(groupName, "SpinSpeed");
	kCurvePeakRate = globalVariables->GetFloatValue(groupName, "CurvePeakRate");
	kDownCurveVerticalRate = globalVariables->GetFloatValue(groupName, "DownCurveVerticalRate");
}

#pragma endregion

#pragma region 初期化

void Boomerang::Initialize(float groundY) {
	// 仮モデル（細長い立方体を回して見せる）。モデルができたら差し替える
	model_.Initialize(Primitive::kCube);
	model_.SetColor({ 1.0f, 0.85f, 0.3f, 1.0f });
	model_.GetTransform().scale = { 1.0f, 0.2f, 0.2f };

	groundY_ = groundY;
	Reset();
}

void Boomerang::Reset() {
	phase_ = Phase::kIdle;
	position_ = { 0.0f, 0.0f, 0.0f };
	timer_ = 0.0f;
	speed_ = 0.0f;
	hasHit_ = false;
}

#pragma endregion

#pragma region 更新

void Boomerang::Update(const Vector3& ownerPosition) {
	switch (phase_) {
	case Phase::kOutward:
		UpdateOutward();
		break;
	case Phase::kReturn:
		UpdateReturn(ownerPosition);
		break;
	case Phase::kIdle:
	default:
		break;
	}

	// 見た目：位置を合わせ、飛行中は回し続ける
	Transform3D& transform = model_.GetTransform();
	transform.translate = position_;
	if (IsFlying()) {
		transform.rotate.z += kSpinSpeed;
	}
	model_.Update();
}

void Boomerang::UpdateModel() {
	model_.Update();
}

void Boomerang::Throw(const Vector3& start, const Vector3& target, float curveOffset) {
	if (IsFlying()) {
		return;
	}
	controlPoints_ = MakeControlPoints(start, target, curveOffset);
	position_ = start;
	timer_ = 0.0f;
	speed_ = 0.0f;
	hasHit_ = false;
	phase_ = Phase::kOutward;
}

void Boomerang::Repel() {
	if (phase_ == Phase::kOutward) {
		StartReturn();
	}
}

std::array<Vector3, 3> Boomerang::MakeControlPoints(const Vector3& start, const Vector3& target, float curveOffset) {
	// 始点と目標点を結ぶ線に垂直な単位ベクトル（線を 90 度回したもの）。上側（Y が正）を正の向きにそろえる
	const Vector3 chord = target - start;
	Vector3 normal = { -chord.y, chord.x, 0.0f };
	if (Length(normal) < 1e-6f) {
		// 始点と目標点が同じ場所なら真上を使う
		normal = { 0.0f, 1.0f, 0.0f };
	} else {
		normal = Normalize(normal);
		if (normal.y < 0.0f || (normal.y == 0.0f && normal.x < 0.0f)) {
			normal = -normal;
		}
	}

	// 2 次ベジェ曲線は制御点の方へ半分だけ寄るので、制御点は 2 倍ずらして曲線の膨らみを curveOffset にそろえる。
	// ずらす元の点を線の真ん中から目標点寄りにすると、曲線の山も目標点寄りになる（最初はまっすぐ気味、最後に回り込む）
	Vector3 middle = start + chord * kCurvePeakRate + normal * (curveOffset * 2.0f);

	// 下向きに曲げたときは、制御点を目標点の真下へ寄せる。2 次ベジェ曲線の終わりの向きは「制御点 → 目標点」なので、
	// 真下に置くと最後は真上へ上がって目標点に入る（線に垂直に膨らませるだけだと、終わりが横へそれて入る）
	if (curveOffset < 0.0f) {
		const Vector3 below = { target.x, target.y + curveOffset * 2.0f, 0.0f };
		middle = Lerp(middle, below, kDownCurveVerticalRate);
	}
	return std::array<Vector3, 3>{ start, middle, target };
}

void Boomerang::UpdateOutward() {
	timer_ += kFrameTime;
	const float t = std::clamp(timer_ / kOutwardDuration, 0.0f, 1.0f);

	// 終わりにかけて減速しながら曲線をなぞる
	position_ = Bezier(controlPoints_[0], controlPoints_[1], controlPoints_[2], EaseOutQuad(t));

	// 地面に触れたら、そこで折り返す（盾に弾かれたときと同じ）
	if (ResolveGroundCollision()) {
		StartReturn();
		return;
	}

	// 目標点に着いたら復路へ
	if (t >= 1.0f) {
		StartReturn();
	}
}

bool Boomerang::ResolveGroundCollision() {
	// 判定の球の下端が地面に届いたら、地面の上へ押し戻す
	const float minY = groundY_ + kRadius;
	if (position_.y <= minY) {
		position_.y = minY;
		return true;
	}
	return false;
}

void Boomerang::StartReturn() {
	phase_ = Phase::kReturn;
	speed_ = kReturnStartSpeed;
	hasHit_ = false;  // 復路でもう一度当たれる
}

void Boomerang::UpdateReturn(const Vector3& ownerPosition) {
	const Vector3 toOwner = ownerPosition - position_;
	const float distance = Length(toOwner);

	// 手元まで戻ったら回収
	if (distance <= kCatchDistance) {
		phase_ = Phase::kIdle;
		position_ = ownerPosition;
		return;
	}

	// 加速しながら持ち主へ向かう（行き過ぎないよう、1 フレームの移動量は残りの距離で頭打ち）
	speed_ = std::clamp(speed_ + kReturnAcceleration, 0.0f, kReturnMaxSpeed);
	const float step = (speed_ < distance) ? speed_ : distance;
	position_ += Normalize(toOwner) * step;

	// 復路でも地面にはもぐらない（高さだけ止めて、そのまま持ち主へ向かう）
	ResolveGroundCollision();
}

#pragma endregion

#pragma region 描画

void Boomerang::Draw() const {
	// 手元にあるときは描かない（プレイヤーが持っている想定）
	if (!IsFlying()) {
		return;
	}
	model_.Draw();
}

#ifdef USE_IMGUI
void Boomerang::DrawImGui() {
	if (ImGui::TreeNode("Boomerang")) {
		const char* phaseName = "idle";
		if (phase_ == Phase::kOutward) {
			phaseName = "outward";
		} else if (phase_ == Phase::kReturn) {
			phaseName = "return";
		}
		ImGui::Text("phase: %s  position: %.2f, %.2f  speed: %.3f  hasHit: %s", phaseName, position_.x, position_.y, speed_, hasHit_ ? "true" : "false");
		ImGui::TreePop();
	}
}
#endif

#pragma endregion
