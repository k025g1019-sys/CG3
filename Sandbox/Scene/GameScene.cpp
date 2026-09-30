#include "Sandbox/Scene/GameScene.h"

#include "Engine/Rendering/Mesh.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void GameScene::OnInitializeObjects() {
	// --- plane.obj（左下。左上はスプライトに隠れるため下段に置く）---
	plane_.Initialize("resources/plane.obj");
	plane_.GetTransform().translate = { -2.25f, -1.45f, 6.0f };

	// --- bunny.obj（スタンフォードバニー。mtl由来のuvCheckerで描かれる）---
	bunny_.Initialize("resources/bunny.obj");
	bunny_.GetTransform().translate = { -0.1f, -0.4f, 6.0f };

	// --- multiMaterial.obj（2サブメッシュ・2マテリアル。monsterBallとuvCheckerの2色になる）---
	multiMaterial_.Initialize("resources/multiMaterial.obj");
	multiMaterial_.GetTransform().translate = { -0.2f, -1.5f, 6.0f };

	// --- suzanne.obj（UVなし。mtlにmap_Kdも無いので白テクスチャ＋ライティングの単色で描かれる）---
	suzanne_.Initialize("resources/suzanne.obj");
	suzanne_.GetTransform().translate = { 1.7f, 3.0f, 6.0f };

	// --- fence ---
	fence_.Initialize("resources/fence.obj");
	fence_.GetTransform().translate = { 0.0f, 0.0f, 0.0f };

	// --- スプライト ---
	sprite_.Initialize(textureHandles_[spriteTextureIndex_], { 640.0f, 360.0f });
}

void GameScene::OnUpdateObjects() {
	plane_.Update();
	bunny_.Update();
	multiMaterial_.Update();
	suzanne_.Update();
	fence_.Update();

	// スプライト（正射影・2Dカリング。基準解像度との比に応じて等比スケールされる）
	sprite_.Update();
}

#ifndef NDEBUG
void GameScene::AppendPickTargets(std::vector<DebugCamera::PickTarget>& targets) const {
	for (const Object3D* object : { &plane_, &bunny_, &multiMaterial_, &suzanne_, &fence_ }) {
		Sphere sphere = object->CalcWorldBoundingSphere();
		targets.push_back({ sphere.center, sphere.radius });
	}
}
#endif

void GameScene::AppendPadTargets(std::vector<PadObjectController::Target>& targets) {
	targets.push_back({ "Plane", &plane_ });
	targets.push_back({ "Bunny", &bunny_ });
	targets.push_back({ "MultiMaterial", &multiMaterial_ });
	targets.push_back({ "Suzanne", &suzanne_ });
	targets.push_back({ "fence" , &fence_ });
}

#ifdef USE_IMGUI
void GameScene::OnDrawObjectsImGui() {
	// OBJモデル共通の編集UI（Transform・色・ライティング・テクスチャ・サブメッシュ表示）
	struct ModelEntry {
		const char* label;
		Engine::Object3D* object;
		int* textureIndex;
	};
	const ModelEntry entries[] = {
		{ "Plane",         &plane_,         &planeTextureIndex_ },
		{ "Bunny",         &bunny_,         &bunnyTextureIndex_ },
		{ "MultiMaterial", &multiMaterial_, &multiMaterialTextureIndex_ },
		{ "Suzanne",       &suzanne_,       &suzanneTextureIndex_ },
		{ "Fence",         &fence_,         &fenceTextureIndex_ },
	};

	for (const ModelEntry& entry : entries) {
		if (ImGui::TreeNode(entry.label)) {
			ImGui::PushID(entry.label);

			Transform3D& transform = entry.object->GetTransform();
			ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
			ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
			ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
			ImGui::Separator();

			DrawMaterialEditor(*entry.object);
			DrawModelTextureCombo(*entry.object, *entry.textureIndex);

			ImGui::Separator();
			DrawSubMeshInfo(*entry.object->GetMesh());

			ImGui::PopID();
			ImGui::TreePop();
		}
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
	ImGui::Text("Plane         (sphere) : %s", VisibilityText(plane_.GetVisibility()));
	ImGui::Text("Bunny         (sphere) : %s", VisibilityText(bunny_.GetVisibility()));
	ImGui::Text("MultiMaterial (sphere) : %s", VisibilityText(multiMaterial_.GetVisibility()));
	ImGui::Text("Suzanne       (sphere) : %s", VisibilityText(suzanne_.GetVisibility()));
	ImGui::Text("Fence         (sphere) : %s", VisibilityText(fence_.GetVisibility()));
	ImGui::Text("Sprite        (2D AABB): %s", VisibilityText(sprite_.GetVisibility()));
}
#endif

void GameScene::OnDrawObjects() {
	plane_.Draw();
	bunny_.Draw();
	multiMaterial_.Draw();
	suzanne_.Draw();
	fence_.Draw();

	// スプライト（drawSprite_がfalse、または画面外なら描かれない）
	if (drawSprite_) {
		sprite_.Draw();
	}
}
