#include "Sandbox/Scene/AxisScene.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Rendering/VertexData.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void AxisScene::OnInitializeObjects() {
	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

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
	triangle_.Initialize(&triangleMesh_, textureHandles_[triangleTextureIndex_]);
	triangle_.GetTransform().translate = { 2.6f, 3.0f, 6.0f };

	// --- axis.obj ---
	axis_.Initialize("resources/axis.obj");
	axis_.GetTransform().translate = { 1.4f, 2.4f, 6.0f };
	axis_.GetTransform().rotate.y = 3.1415f;

	// --- teapot.obj（ユタ・ティーポット。mtl由来のcheckerBoardで描かれる）---
	teapot_.Initialize("resources/teapot.obj");
	teapot_.GetTransform().translate = { -1.6f, 1.1f, 6.0f };

	// --- multiMesh.obj（2サブメッシュ・1マテリアル）---
	multiMesh_.Initialize("resources/multiMesh.obj");
	multiMesh_.GetTransform().translate = { -1.2f, -1.7f, 9.0f };

	// --- 球 ---
	sphereMesh_.CreateSphere(device, subdivision_);
	sphere_.Initialize(&sphereMesh_, textureHandles_[sphereTextureIndex_]);
	sphere_.GetTransform().translate = { 2.2f, 0.7f, 6.0f };
	sphere_.GetTransform().rotate.y = 4.9f;

	// ---- GameSceneから移したオブジェクト ----
	// 上のオブジェクトと画面上で重ならないよう、左右の空いている所に置く。
	// 大きいモデルは奥に置いて見かけを小さくし、左右の空きに収める。

	// --- plane.obj（右下）---
	plane_.Initialize("resources/plane.obj");
	plane_.GetTransform().translate = { 4.5f, -1.4f, 6.0f };

	// --- bunny.obj（スタンフォードバニー。右の中段。mtl由来のuvCheckerで描かれる）---
	bunny_.Initialize("resources/bunny.obj");
	bunny_.GetTransform().translate = { 4.65f, 0.9f, 6.0f };

	// --- multiMaterial.obj（2サブメッシュ・2マテリアル。左の奥。monsterBallとuvCheckerの2色になる）---
	multiMaterial_.Initialize("resources/multiMaterial.obj");
	multiMaterial_.GetTransform().translate = { -10.8f, 1.3f, 24.0f };

	// --- suzanne.obj（UVなし。右上の少し奥。mtlにmap_Kdも無いので白テクスチャ＋ライティングの単色で描かれる）---
	suzanne_.Initialize("resources/suzanne.obj");
	suzanne_.GetTransform().translate = { 5.6f, 4.2f, 10.0f };

	// --- fence.obj（透明部分をdiscardで抜く。左下の奥）---
	fence_.Initialize("resources/fence.obj");
	fence_.GetTransform().translate = { -9.55f, -5.95f, 24.0f };

	// --- スプライト（画面の左上。3Dオブジェクトに重ならないよう、画面の縦横1/4の大きさにする）---
	sprite_.Initialize(textureHandles_[spriteTextureIndex_], { 320.0f, 180.0f });
}

void AxisScene::OnUpdateObjects() {
	triangle_.Update();
	axis_.Update();
	teapot_.Update();
	multiMesh_.Update();
	sphere_.Update();

	plane_.Update();
	bunny_.Update();
	multiMaterial_.Update();
	suzanne_.Update();
	fence_.Update();

	// スプライト（正射影・2Dカリング。基準解像度との比に応じて等比スケールされる）
	sprite_.Update();
}

#ifndef NDEBUG
void AxisScene::AppendPickTargets(std::vector<DebugCamera::PickTarget>& targets) const {
	for (const Object3D* object : {
			&triangle_, &axis_, &teapot_, &multiMesh_, &sphere_,
			&plane_, &bunny_, &multiMaterial_, &suzanne_, &fence_ }) {
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
	targets.push_back({ "Plane", &plane_ });
	targets.push_back({ "Bunny", &bunny_ });
	targets.push_back({ "MultiMaterial", &multiMaterial_ });
	targets.push_back({ "Suzanne", &suzanne_ });
	targets.push_back({ "fence" , &fence_ });
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
		int* textureIndex;
	};
	const ModelEntry entries[] = {
		{ "Axis",      &axis_,      &axisTextureIndex_ },
		{ "Teapot",    &teapot_,    &teapotTextureIndex_ },
		{ "MultiMesh", &multiMesh_, &multiMeshTextureIndex_ },
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
			DrawSubMeshInfo(*entry.object->GetMesh());

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

	// ----GameSceneから移したOBJモデル（共通の編集UI：Transform・色・ライティング・テクスチャ・サブメッシュ表示）----
	const ModelEntry movedEntries[] = {
		{ "Plane",         &plane_,         &planeTextureIndex_ },
		{ "Bunny",         &bunny_,         &bunnyTextureIndex_ },
		{ "MultiMaterial", &multiMaterial_, &multiMaterialTextureIndex_ },
		{ "Suzanne",       &suzanne_,       &suzanneTextureIndex_ },
		{ "Fence",         &fence_,         &fenceTextureIndex_ },
	};

	for (const ModelEntry& entry : movedEntries) {
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

void AxisScene::OnDrawExtraImGui() {
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

void AxisScene::OnDrawCullingImGui() {
	ImGui::Text("Triangle      (sphere) : %s", VisibilityText(triangle_.GetVisibility()));
	ImGui::Text("Axis          (sphere) : %s", VisibilityText(axis_.GetVisibility()));
	ImGui::Text("Teapot        (sphere) : %s", VisibilityText(teapot_.GetVisibility()));
	ImGui::Text("MultiMesh     (sphere) : %s", VisibilityText(multiMesh_.GetVisibility()));
	ImGui::Text("Sphere        (sphere) : %s", VisibilityText(sphere_.GetVisibility()));
	ImGui::Text("Plane         (sphere) : %s", VisibilityText(plane_.GetVisibility()));
	ImGui::Text("Bunny         (sphere) : %s", VisibilityText(bunny_.GetVisibility()));
	ImGui::Text("MultiMaterial (sphere) : %s", VisibilityText(multiMaterial_.GetVisibility()));
	ImGui::Text("Suzanne       (sphere) : %s", VisibilityText(suzanne_.GetVisibility()));
	ImGui::Text("Fence         (sphere) : %s", VisibilityText(fence_.GetVisibility()));
	ImGui::Text("Sprite        (2D AABB): %s", VisibilityText(sprite_.GetVisibility()));
}
#endif

void AxisScene::OnDrawObjects() {
	triangle_.Draw();
	axis_.Draw();
	teapot_.Draw();
	multiMesh_.Draw();
	sphere_.Draw();

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
