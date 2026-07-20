#include "Game/Object/PadObjectController.h"

#include "Engine/Input/Input.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

// 1フレームあたりの操作量（プロジェクト全体と同じ60fps前提のフレームベース値）
constexpr float kMoveSpeed = 0.05f;      // スティック最大倒しでのXZ移動量
constexpr float kVerticalSpeed = 0.05f;  // B/AボタンでのY移動量
constexpr float kRotateSpeed = 0.03f;    // スティック最大倒しでの回転量（ラジアン）
constexpr float kScaleRate = 0.02f;      // トリガー最大踏み込みでの拡縮率
constexpr float kMinScale = 0.01f;       // 縮小の下限（消失を防ぎ、縮め過ぎても戻せるようにする）
constexpr float kMaxScale = 100.0f;      // 拡大の上限

// スケールの絶対値を[kMinScale, kMaxScale]へクランプする（符号は維持。
// ImGuiで負のスケールにした場合も反転状態のまま拡縮できるようにする）。
// 明示比較：Windows.hのmin/maxマクロ回避
float ClampScale(float value) {
	const float sign = (value < 0.0f) ? -1.0f : 1.0f;
	float magnitude = (value < 0.0f) ? -value : value;
	if (magnitude < kMinScale) { magnitude = kMinScale; }
	if (magnitude > kMaxScale) { magnitude = kMaxScale; }
	return sign * magnitude;
}

}  // namespace

void PadObjectController::SetTargets(std::vector<Target> targets) {
	// 差し替え前に選択していたオブジェクトを覚えておき、新しいリストから同じものを探す
	// （立方体の削除でインデックスがずれても選択対象が変わらないようにする）
	Engine::Object3D* previous = nullptr;
	if (selectedIndex_ >= 0 && selectedIndex_ < int(targets_.size())) {
		previous = targets_[size_t(selectedIndex_)].object;
	}

	targets_ = std::move(targets);

	if (previous != nullptr) {
		for (int i = 0; i < int(targets_.size()); ++i) {
			if (targets_[size_t(i)].object == previous) {
				selectedIndex_ = i;
				return;
			}
		}
	}
	// 以前の選択対象が消えた場合（削除など）のみ範囲内へ丸める
	if (selectedIndex_ >= int(targets_.size())) {
		selectedIndex_ = targets_.empty() ? 0 : int(targets_.size()) - 1;
	}
}

void PadObjectController::Update() {
	Input* input = Input::GetInstance();
	if (targets_.empty() || !input->IsPadConnected()) {
		return;
	}

	// RB / LB で選択オブジェクトを切り替える（端まで行くと反対側へループ）
	const int count = int(targets_.size());
	if (input->IsPadTrigger(kPadRightShoulder)) {
		selectedIndex_ = (selectedIndex_ + 1) % count;
	}
	if (input->IsPadTrigger(kPadLeftShoulder)) {
		selectedIndex_ = (selectedIndex_ + count - 1) % count;
	}

	Transform3D& transform = targets_[size_t(selectedIndex_)].object->GetTransform();

	// 左スティック：XZ平面の平行移動（ワールド軸基準。上に倒すと奥＝+Z）
	const Vector2 leftStick = input->GetLeftStick();
	transform.translate.x += leftStick.x * kMoveSpeed;
	transform.translate.z += leftStick.y * kMoveSpeed;

	// B / A：Y軸の上昇・下降
	if (input->IsPadPress(kPadX)) {
		transform.translate.y += kVerticalSpeed;
	}
	if (input->IsPadPress(kPadA)) {
		transform.translate.y -= kVerticalSpeed;
	}

	// 右スティック：回転（横=Y軸ヨー / 縦=X軸ピッチ。上に倒すと頭が奥へ倒れる向き）
	const Vector2 rightStick = input->GetRightStick();
	transform.rotate.y -= rightStick.x * kRotateSpeed;
	transform.rotate.x += rightStick.y * kRotateSpeed;

	// RT / LT：拡大・縮小（踏み込み量に応じた倍率を毎フレーム掛ける。全軸等倍）
	// 未入力時は掛けない（ImGuiで編集した値をクランプで上書きしないようにする）
	const float triggerDelta = input->GetRightTrigger() - input->GetLeftTrigger();
	if (triggerDelta != 0.0f) {
		const float scaleFactor = 1.0f + triggerDelta * kScaleRate;
		transform.scale.x = ClampScale(transform.scale.x * scaleFactor);
		transform.scale.y = ClampScale(transform.scale.y * scaleFactor);
		transform.scale.z = ClampScale(transform.scale.z * scaleFactor);
	}
}

#ifdef USE_IMGUI
void PadObjectController::DrawImGui() {
	ImGui::Text("Gamepad: %s",
		Input::GetInstance()->IsPadConnected() ? "connected" : "not connected");

	// 選択中オブジェクト（RB/LBのほか、ここからも切り替えられる）
	if (!targets_.empty()) {
		if (ImGui::BeginCombo("Pad Target", targets_[size_t(selectedIndex_)].label.c_str())) {
			for (int i = 0; i < int(targets_.size()); ++i) {
				const bool selected = (i == selectedIndex_);
				if (ImGui::Selectable(targets_[size_t(i)].label.c_str(), selected)) {
					selectedIndex_ = i;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	if (ImGui::TreeNode("Pad Controls")) {
		ImGui::BulletText("L Stick : Move XZ");
		ImGui::BulletText("R Stick : Rotate (yaw / pitch)");
		ImGui::BulletText("RT / LT : Scale up / down");
		ImGui::BulletText("B / A   : Move up / down");
		ImGui::BulletText("RB / LB : Next / prev target");
		ImGui::BulletText("Menu    : Switch demo scene (not in stereo demo)");
		ImGui::BulletText("View    : Toggle stereo demo");
		ImGui::TreePop();
	}

	ImGui::Separator();
}
#endif
