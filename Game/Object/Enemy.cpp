#include "Game/Object/Enemy.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void Enemy::Initialize() {
	// 仮モデル（組み込みの立方体）。モデルができたら model_.Initialize("resources/enemy.obj") に差し替える
	model_.Initialize(Primitive::kCube);
	model_.SetColor({ 1.0f, 0.4f, 0.3f, 1.0f });
	model_.GetTransform().translate = { 0.0f, kGroundY, 0.0f };
}

void Enemy::Update() {

	Transform3D& transform = model_.GetTransform();

	// ジャンプ
	if (isGrounded_) {
		jumpVelocity_ = kJumpPower;
		isGrounded_ = false;
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

void Enemy::Draw() const {
	model_.Draw();
}

#ifdef USE_IMGUI
void Enemy::DrawImGui() {
	if (ImGui::TreeNode("Enemy")) {
		ImGui::DragFloat3("translate", &model_.GetTransform().translate.x, 0.01f);
		ImGui::Text("jumpVelocity: %.3f", jumpVelocity_);
		ImGui::Text("isGrounded: %s", isGrounded_ ? "true" : "false");
		ImGui::TreePop();
	}
}
#endif
