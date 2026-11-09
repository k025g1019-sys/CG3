#include "Scene/PlaneScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void PlaneScene::OnInitializeObjects() {
	// --- 板ポリ（plane.obj）をkNumInstance枚まとめて描けるようにする ---
	planes_.Initialize("resources/plane.obj", kNumInstance);

	// それぞれ位置が少しずつずれるように初期化する
	for (uint32_t index = 0; index < kNumInstance; ++index) {
		Transform3D& transform = planes_.GetTransform(index);
		transform.scale = { 1.0f, 1.0f, 1.0f };
		transform.rotate = { 0.0f, 0.0f, 0.0f };
		transform.translate = { float(index) * 0.1f, float(index) * 0.1f, float(index) * 0.1f };
	}
}

void PlaneScene::OnUpdateObjects() {
	// 各インスタンスのワールド行列をStructuredBufferへ書き込む
	planes_.Update();
}

void PlaneScene::OnDrawObjects() {
	// 板ポリkNumInstance枚を1回の描画命令（DrawInstanced）で描く
	planes_.Draw();
}

#ifndef NDEBUG
void PlaneScene::AppendPickTargets(std::vector<DebugCamera::PickTarget>& /*targets*/) const {
	// 板ポリはObject3Dではないため、ピッキングの対象にしない
}
#endif

void PlaneScene::AppendPadTargets(std::vector<PadObjectController::Target>& /*targets*/) {
	// 板ポリはObject3Dではないため、パッド操作の対象にしない
}

#ifdef USE_IMGUI
void PlaneScene::OnDrawObjectsImGui() {
	if (ImGui::TreeNode("Planes (Instancing)")) {
		ImGui::PushID("Planes");

		// 描画するインスタンス数（確保した数まで。少ない分には問題ない）
		int instanceCount = static_cast<int>(planes_.GetInstanceCount());
		if (ImGui::SliderInt("Instance Count", &instanceCount,
				0, static_cast<int>(planes_.GetMaxInstanceCount()), "%d", ImGuiSliderFlags_AlwaysClamp)) {
			planes_.SetInstanceCount(static_cast<uint32_t>(instanceCount));
		}

		// 全インスタンス共通の色
		ImGui::ColorEdit4("Color", &planes_.GetMaterial().color.x);
		ImGui::Separator();

		// インスタンスごとのTransform（IDはインデックスにして、開閉状態を保つ）
		for (uint32_t index = 0; index < planes_.GetMaxInstanceCount(); ++index) {
			if (ImGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(index)), "Instance %u", index)) {
				Transform3D& transform = planes_.GetTransform(index);
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

void PlaneScene::OnDrawCullingImGui() {
	// Instancingでまとめて描くため、インスタンスごとの視錐台カリングはしない
	ImGui::Text("Planes (instancing): not culled");
}
#endif
