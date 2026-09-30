#include "Game/Object/Player.h"

#include "Engine/Input/Input.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void Player::Initialize() {
	// 仮モデル（組み込みの立方体）。モデルができたら model_.Initialize("resources/player.obj") に差し替える
	model_.Initialize(Primitive::kCube);
	model_.SetColor({ 1.0f, 0.4f, 0.3f, 1.0f });
	model_.GetTransform().translate = { 0.0f, kGroundY, 0.0f };
}

void Player::Update() {
	Input* input = Input::GetInstance();

	Transform3D& transform = model_.GetTransform();

	// Wキーを押した瞬間にジャンプ
	if (input->IsTrigger(DIK_W) && isGrounded_) {
		jumpVelocity_ = kJumpPower;
		isGrounded_ = false;
	}
	//if (input->IsPress(DIK_S)) {
	//	transform.translate.y -= kSpeed;
	//}
	if (input->IsPress(DIK_A)) {
		transform.translate.x -= kSpeed;
	}
	if (input->IsPress(DIK_D)) {
		transform.translate.x += kSpeed;
	}
	// 空中にいる場合
	if (!isGrounded_) {
		// 上方向へ移動
		transform.translate.y += jumpVelocity_;

		// 重力
		jumpVelocity_ -= kGravity;

		// 地面に着いたか
		if (transform.translate.y <= kGroundY) {
			transform.translate.y = kGroundY;
			jumpVelocity_ = 0.0f;
			isGrounded_ = true;
		}
	}

	model_.Update();
}

void Player::Draw() const {
	model_.Draw();
}

#ifdef USE_IMGUI
void Player::DrawImGui() {
	if (ImGui::TreeNode("Player")) {
		ImGui::DragFloat3("translate", &model_.GetTransform().translate.x, 0.01f);
		ImGui::Text("jumpVelocity: %.3f", jumpVelocity_);
		ImGui::Text("isGrounded: %s", isGrounded_ ? "true" : "false");
		ImGui::TreePop();
	}
}
#endif
