#include "Sandbox/Scene/AxisScene.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Rendering/VertexData.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void AxisScene::OnInitialize(ID3D12Device* device) {
	// --- 三角形（2枚。2枚目は1枚目を貫通する）---
	VertexData triangleVertices[6]{};
	triangleVertices[0].position = { -0.5f, -0.5f, 0.0f, 1.0f }; // 左下
	triangleVertices[0].texcoord = { 0.0f, 1.0f };
	triangleVertices[1].position = { 0.0f, 0.5f, 0.0f, 1.0f }; // 上
	triangleVertices[1].texcoord = { 0.5f, 0.0f };
	triangleVertices[2].position = { 0.5f, -0.5f, 0.0f, 1.0f }; // 右下
	triangleVertices[2].texcoord = { 1.0f, 1.0f };
	triangleVertices[3].position = { -0.5f, -0.5f, 0.5f, 1.0f }; // 左下
	triangleVertices[3].texcoord = { 0.0f, 1.0f };
	triangleVertices[4].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 上
	triangleVertices[4].texcoord = { 0.5f, 0.0f };
	triangleVertices[5].position = { 0.5f, -0.5f, -0.5f, 1.0f }; // 右下
	triangleVertices[5].texcoord = { 1.0f, 1.0f };
	for (VertexData& vertex : triangleVertices) {
		vertex.normal = { 0.0f, 0.0f, -1.0f };
	}
	triangleMesh_.Create(device, triangleVertices, 6);
	triangle_.Initialize(device, &triangleMesh_, textureHandles_[triangleTextureIndex_]);
	triangle_.GetTransform().translate = { 2.6f, 3.0f, 6.0f };

	// --- axis.obj ---
	axisMesh_.CreateFromObj(device, "resources", "axis.obj");
	axis_.Initialize(device, &axisMesh_, textureHandles_[0]);
	axis_.GetTransform().translate = { 1.4f, 2.4f, 6.0f };
	axis_.GetTransform().rotate.y = 3.1415f;

	// --- teapot.obj（ユタ・ティーポット。mtl由来のcheckerBoardで描かれる）---
	teapotMesh_.CreateFromObj(device, "resources", "teapot.obj");
	teapot_.Initialize(device, &teapotMesh_, textureHandles_[0]);
	teapot_.GetTransform().translate = { -1.6f, 1.1f, 6.0f };

	// --- multiMesh.obj（2サブメッシュ・1マテリアル）---
	multiMeshMesh_.CreateFromObj(device, "resources", "multiMesh.obj");
	multiMesh_.Initialize(device, &multiMeshMesh_, textureHandles_[0]);
	multiMesh_.GetTransform().translate = { -1.2f, -1.7f, 9.0f };

	// --- 球 ---
	sphereMesh_.CreateSphere(device, subdivision_);
	sphere_.Initialize(device, &sphereMesh_, textureHandles_[sphereTextureIndex_]);
	sphere_.GetTransform().translate = { 2.2f, 0.7f, 6.0f };
	sphere_.GetTransform().rotate.y = 4.9f;
}

void AxisScene::OnUpdate(const Frustum3D& frustum, float viewWidth, float viewHeight) {
	// このシーンに2Dオブジェクトはない（描画先矩形の大きさは使わない）
	(void)viewWidth;
	(void)viewHeight;

	triangle_.Update(frustum);
	axis_.Update(frustum);
	teapot_.Update(frustum);
	multiMesh_.Update(frustum);
	sphere_.Update(frustum);
}

#ifndef NDEBUG
void AxisScene::AppendPickTargets(std::vector<DebugCamera::PickTarget>& targets) const {
	for (const Object3D* object : { &triangle_, &axis_, &teapot_, &multiMesh_, &sphere_ }) {
		Sphere sphere = object->CalcWorldBoundingSphere();
		targets.push_back({ sphere.center, sphere.radius });
	}
}
#endif

void AxisScene::AppendPadTargets(std::vector<PadObjectController::Target>& targets) {
	targets.push_back({ "Triangle", &triangle_ });
	targets.push_back({ "Axis", &axis_ });
	targets.push_back({ "Teapot", &teapot_ });
	targets.push_back({ "MultiMesh", &multiMesh_ });
	targets.push_back({ "Sphere", &sphere_ });
}

#ifdef USE_IMGUI
void AxisScene::OnDrawObjectsImGui() {
	// ----Triangle----
	if (ImGui::TreeNode("Triangle")) {
		ImGui::PushID("Triangle");

		Transform3D& transform = triangle_.GetTransform();
		ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
		ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
		ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
		ImGui::Separator();
		VertexData* vertices = triangleMesh_.GetMappedVertices();
		ImGui::DragFloat4("Vertex0", &vertices[0].position.x, 0.01f);
		ImGui::DragFloat4("Vertex1", &vertices[1].position.x, 0.01f);
		ImGui::DragFloat4("Vertex2", &vertices[2].position.x, 0.01f);
		ImGui::Separator();

		ImGui::ColorEdit4("Color", &triangle_.GetMaterial().color.x);
		DrawLightingModeCombo(triangle_.GetMaterial());

		if (ImGui::Combo("Texture", &triangleTextureIndex_, kTextureItems, kTextureCount)) {
			triangle_.SetTextureHandle(textureHandles_[triangleTextureIndex_]);
		}

		ImGui::PopID();
		ImGui::TreePop();
	}

	// ----OBJモデル（共通の編集UI：Transform・色・ライティング・テクスチャ・サブメッシュ表示）----
	struct ModelEntry {
		const char* label;
		Engine::Object3D* object;
		Engine::Mesh* mesh;
		int* textureIndex;
	};
	const ModelEntry entries[] = {
		{ "Axis",      &axis_,      &axisMesh_,      &axisTextureIndex_ },
		{ "Teapot",    &teapot_,    &teapotMesh_,    &teapotTextureIndex_ },
		{ "MultiMesh", &multiMesh_, &multiMeshMesh_, &multiMeshTextureIndex_ },
	};

	for (const ModelEntry& entry : entries) {
		if (ImGui::TreeNode(entry.label)) {
			ImGui::PushID(entry.label);

			Transform3D& transform = entry.object->GetTransform();
			ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
			ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
			ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
			ImGui::Separator();

			ImGui::ColorEdit4("Color", &entry.object->GetMaterial().color.x);
			DrawLightingModeCombo(entry.object->GetMaterial());
			DrawModelTextureCombo(*entry.object, *entry.textureIndex);

			ImGui::Separator();
			DrawSubMeshInfo(*entry.mesh);

			ImGui::PopID();
			ImGui::TreePop();
		}
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
	ImGui::Text("Triangle  (sphere) : %s", VisibilityText(triangle_.GetVisibility()));
	ImGui::Text("Axis      (sphere) : %s", VisibilityText(axis_.GetVisibility()));
	ImGui::Text("Teapot    (sphere) : %s", VisibilityText(teapot_.GetVisibility()));
	ImGui::Text("MultiMesh (sphere) : %s", VisibilityText(multiMesh_.GetVisibility()));
	ImGui::Text("Sphere    (sphere) : %s", VisibilityText(sphere_.GetVisibility()));
}
#endif

void AxisScene::OnDraw(ID3D12GraphicsCommandList* commandList) {
	triangle_.Draw(commandList);
	axis_.Draw(commandList);
	teapot_.Draw(commandList);
	multiMesh_.Draw(commandList);
	sphere_.Draw(commandList);
}
