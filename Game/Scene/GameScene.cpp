#include "Game/Scene/GameScene.h"

#include "Engine/Rendering/VertexData.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void GameScene::OnInitialize(ID3D12Device* device) {
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
	triangle_.GetTransform().translate = { 2.5f, 0.0f, 0.0f };

	// --- OBJモデル ---
	objMesh_.CreateFromObj(device, "resources", "plane.obj");
	obj_.Initialize(device, &objMesh_, textureHandles_[objTextureIndex_]);
	obj_.GetTransform().rotate.y = 0.0f;

	// --- スプライト ---
	sprite_.Initialize(device, textureHandles_[spriteTextureIndex_], { 640.0f, 360.0f });
}

void GameScene::OnUpdate(const Frustum3D& frustum, float viewWidth, float viewHeight) {
	triangle_.Update(frustum);
	obj_.Update(frustum);

	// スプライト（正射影・2Dカリング。基準解像度との比に応じて等比スケールされる）
	sprite_.Update(viewWidth, viewHeight);
}

#ifndef NDEBUG
void GameScene::AppendPickTargets(std::vector<DebugCamera::PickTarget>& targets) const {
	for (const Object3D* object : { &triangle_, &obj_ }) {
		Sphere sphere = object->CalcWorldBoundingSphere();
		targets.push_back({ sphere.center, sphere.radius });
	}
}
#endif

#ifdef USE_IMGUI
void GameScene::OnDrawObjectsImGui() {
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
}

void GameScene::OnDrawExtraImGui() {
	ImGui::Begin("2D Objects");
	if (ImGui::TreeNode("Square")) {
		ImGui::PushID("Square");

		ImGui::Checkbox("Draw Sprite", &drawSprite_);
		ImGui::Separator();

		Transform3D& transform = sprite_.GetTransform();
		ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
		ImGui::DragFloat3("rotate", &transform.rotate.x, 0.05f);
		ImGui::DragFloat3("translate", &transform.translate.x, 0.35f);
		ImGui::Separator();
		VertexData* vertices = sprite_.GetMappedVertices();
		ImGui::DragFloat4("Vertex0 position", &vertices[0].position.x, 0.2f);
		ImGui::DragFloat4("Vertex1 position", &vertices[1].position.x, 0.2f);
		ImGui::DragFloat4("Vertex2 position", &vertices[2].position.x, 0.2f);
		ImGui::DragFloat4("Vertex3 position", &vertices[3].position.x, 0.2f);

		ImGui::DragFloat2("Vertex0 texcoord", &vertices[0].texcoord.x, 0.2f);
		ImGui::DragFloat2("Vertex1 texcoord", &vertices[1].texcoord.x, 0.2f);
		ImGui::DragFloat2("Vertex2 texcoord", &vertices[2].texcoord.x, 0.2f);
		ImGui::DragFloat2("Vertex3 texcoord", &vertices[3].texcoord.x, 0.2f);

		ImGui::Separator();

		Transform3D& uvTransform = sprite_.GetUVTransform();
		ImGui::DragFloat2("UVTranslate", &uvTransform.translate.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2("UVScale", &uvTransform.scale.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle("UVRotate", &uvTransform.rotate.z);

		ImGui::Separator();

		DrawLightingModeCombo(sprite_.GetMaterial());

		if (ImGui::Combo("Texture", &spriteTextureIndex_, kTextureItems, kTextureCount)) {
			sprite_.SetTextureHandle(textureHandles_[spriteTextureIndex_]);
		}

		ImGui::PopID();
		ImGui::TreePop();
	}
	ImGui::End();
}

void GameScene::OnDrawCullingImGui() {
	ImGui::Text("Triangle (sphere) : %s", VisibilityText(triangle_.GetVisibility()));
	ImGui::Text("Obj      (sphere) : %s", VisibilityText(obj_.GetVisibility()));
	ImGui::Text("Sprite   (2D AABB): %s", VisibilityText(sprite_.GetVisibility()));
}
#endif

void GameScene::OnDraw(ID3D12GraphicsCommandList* commandList) {
	triangle_.Draw(commandList);
	obj_.Draw(commandList);

	// スプライト（drawSprite_がfalse、または画面外なら描かれない）
	if (drawSprite_) {
		sprite_.Draw(commandList);
	}
}
