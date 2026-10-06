#include "Sandbox/Scene/GameScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void GameScene::OnInitializeObjects() {
	// --- 板ポリ（plane.obj）をkNumInstance個まとめて描けるようにする ---
	particles_.Initialize("resources/plane.obj", kNumInstance);

	// それぞれ位置が少しずつずれるように初期化する
	for (uint32_t index = 0; index < kNumInstance; ++index) {
		Transform3D& transform = particles_.GetTransform(index);
		transform.scale = { 1.0f, 1.0f, 1.0f };
		transform.rotate = { 0.0f, 0.0f, 0.0f };
		transform.translate = { float(index) * 0.1f, float(index) * 0.1f, float(index) * 0.1f };
	}
}

void GameScene::OnUpdateObjects() {
	// 各インスタンスのワールド行列をStructuredBufferへ書き込む
	particles_.Update();
}

void GameScene::OnDrawObjects() {
	// 板ポリkNumInstance枚を1回の描画命令（DrawInstanced）で描く
	particles_.Draw();
}

#ifndef NDEBUG
void GameScene::AppendPickTargets(std::vector<DebugCamera::PickTarget>& /*targets*/) const {
	// パーティクルはObject3Dではないため、ピッキングの対象にしない
}
#endif

void GameScene::AppendPadTargets(std::vector<PadObjectController::Target>& /*targets*/) {
	// パーティクルはObject3Dではないため、パッド操作の対象にしない
}

#ifdef USE_IMGUI
void GameScene::OnDrawObjectsImGui() {
	if (ImGui::TreeNode("Particles (Instancing)")) {
		ImGui::PushID("Particles");

		// 描画するインスタンス数（確保した数まで。少ない分には問題ない）
		int instanceCount = static_cast<int>(particles_.GetInstanceCount());
		if (ImGui::SliderInt("Instance Count", &instanceCount,
				0, static_cast<int>(particles_.GetMaxInstanceCount()), "%d", ImGuiSliderFlags_AlwaysClamp)) {
			particles_.SetInstanceCount(static_cast<uint32_t>(instanceCount));
		}

		// 全インスタンス共通の色
		ImGui::ColorEdit4("Color", &particles_.GetMaterial().color.x);
		ImGui::Separator();

		// インスタンスごとのTransform（IDはインデックスにして、開閉状態を保つ）
		for (uint32_t index = 0; index < particles_.GetMaxInstanceCount(); ++index) {
			if (ImGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(index)), "Instance %u", index)) {
				Transform3D& transform = particles_.GetTransform(index);
				ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
				ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
				ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
				ImGui::TreePop();
			}
		}

		ImGui::PopID();
		ImGui::TreePop();
	}
}

void GameScene::OnDrawCullingImGui() {
	// Instancingでまとめて描くため、インスタンスごとの視錐台カリングはしない
	ImGui::Text("Particles (instancing): not culled");
}
#endif
