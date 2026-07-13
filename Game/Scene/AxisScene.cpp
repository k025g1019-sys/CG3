#include "Game/Scene/AxisScene.h"

#include "Engine/Core/DirectXCore.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void AxisScene::OnInitialize(ID3D12Device* device) {
	// --- OBJモデル ---
	objMesh_.CreateFromObj(device, "resources", "axis.obj");
	obj_.Initialize(device, &objMesh_, textureHandles_[objTextureIndex_]);
	obj_.GetTransform().rotate.y = 3.1415f;

	// --- 球 ---
	sphereMesh_.CreateSphere(device, subdivision_);
	sphere_.Initialize(device, &sphereMesh_, textureHandles_[sphereTextureIndex_]);
	sphere_.GetTransform().translate = { 2.5f, 0.3f, 0.0f };
	sphere_.GetTransform().rotate.y = 4.9f;
}

void AxisScene::OnUpdate(const Frustum3D& frustum, float viewWidth, float viewHeight) {
	// このシーンに2Dオブジェクトはない（描画先矩形の大きさは使わない）
	(void)viewWidth;
	(void)viewHeight;

	obj_.Update(frustum);
	sphere_.Update(frustum);
}

#ifndef NDEBUG
void AxisScene::AppendPickTargets(std::vector<DebugCamera::PickTarget>& targets) const {
	for (const Object3D* object : { &obj_, &sphere_ }) {
		Sphere sphere = object->CalcWorldBoundingSphere();
		targets.push_back({ sphere.center, sphere.radius });
	}
}
#endif

#ifdef USE_IMGUI
void AxisScene::OnDrawObjectsImGui() {
	// ----Obj----
	if (ImGui::TreeNode("Obj")) {
		ImGui::PushID("Obj");

		Transform3D& transform = obj_.GetTransform();
		ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
		ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
		ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
		ImGui::Separator();

		ImGui::ColorEdit4("Color", &obj_.GetMaterial().color.x);
		DrawLightingModeCombo(obj_.GetMaterial());

		if (ImGui::Combo("Texture", &objTextureIndex_, kTextureItems, kTextureCount)) {
			obj_.SetTextureHandle(textureHandles_[objTextureIndex_]);
		}

		ImGui::PopID();
		ImGui::TreePop();
	}

	// ----Sphere----
	if (ImGui::TreeNode("Sphere")) {
		ImGui::PushID("Sphere");

		Transform3D& transform = sphere_.GetTransform();
		ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
		ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
		ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
		ImGui::Separator();

		ImGui::DragInt("Sphere Subdivision", reinterpret_cast<int*>(&subdivision_), 1, 3, 128);

		if (subdivision_ != prevSubdivision_) {
			// FenceでGPU完了待ちをしてから差し替える（旧頂点バッファは自動開放）
			DirectXCore::GetInstance()->WaitForGPU();

			sphereMesh_.CreateSphere(DirectXCore::GetInstance()->GetDevice(), subdivision_);

			prevSubdivision_ = subdivision_;
		}
		ImGui::Separator();

		ImGui::ColorEdit4("Color", &sphere_.GetMaterial().color.x);
		DrawLightingModeCombo(sphere_.GetMaterial());

		if (ImGui::Combo("Texture", &sphereTextureIndex_, kTextureItems, kTextureCount)) {
			sphere_.SetTextureHandle(textureHandles_[sphereTextureIndex_]);
		}

		ImGui::PopID();
		ImGui::TreePop();
	}
}

void AxisScene::OnDrawCullingImGui() {
	ImGui::Text("Obj      (sphere) : %s", VisibilityText(obj_.GetVisibility()));
	ImGui::Text("Sphere   (sphere) : %s", VisibilityText(sphere_.GetVisibility()));
}
#endif

void AxisScene::OnDraw(ID3D12GraphicsCommandList* commandList) {
	obj_.Draw(commandList);
	sphere_.Draw(commandList);
}
