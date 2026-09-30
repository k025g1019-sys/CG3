#include "Sandbox/Scene/DemoSceneBase.h"

#include "Engine/Audio/Audio.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Math/Matrix4x4.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

const char* const DemoSceneBase::kTextureItems[kTextureCount] = {
	"uvChecker",
	"monsterBall"
};

void DemoSceneBase::OnInitialize() {
	// --- シーン共通のテクスチャ ---
	textureHandles_[0] = TextureManager::GetInstance()->Load("resources/uvChecker.png");
	textureHandles_[1] = TextureManager::GetInstance()->Load("resources/monsterBall.png");

	// --- 天球（背景。ライティング無効・カリング無効PSO）---
	skydome_.Initialize();

	// --- カメラ初期位置（派生シーンがOnInitializeObjectsで上書きしてもよい）---
	camera_.GetTransform().rotate = { 0.04f, 0.0f, 0.0f };
	camera_.GetTransform().translate = { 0.0f, 1.7f, -10.0f };

	// --- サウンド読み込み ---
	soundHandle_ = Audio::GetInstance()->LoadWave("resources/Alarm01.wav");
	Audio::GetInstance()->SetVolume(soundHandle_, soundVolume_);

	// --- 選択オブジェクトの回転軸ギズモ ---
	axisGizmo_.Initialize();

	// --- シーン固有のリソース生成 ---
	OnInitializeObjects();
}

void DemoSceneBase::OnUpdate() {
	// スペースキーを押した瞬間にサウンド再生（Enterはデバッグカメラの有効・無効切り替えに割り当て）
	if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
		Audio::GetInstance()->Play(soundHandle_);
	}

	// --- パッドで選択オブジェクトを操作（対象リストは毎フレーム組み立てる）---
	// オブジェクトの更新（OnUpdateObjects）より前に反映し、操作が同じフレームの描画に効くようにする
	std::vector<PadObjectController::Target> padTargets;
	AppendPadTargets(padTargets);
	padController_.SetTargets(std::move(padTargets));
	padController_.Update();

	// 選択オブジェクトの回転軸ギズモを追従させる（定数バッファ書き込みは毎フレームここだけ）
	axisGizmo_.Update(padController_.GetSelectedObject());

	// --- シーン固有オブジェクトの更新 ---
	OnUpdateObjects();

#ifndef NDEBUG
	// --- デバッグカメラ更新（Release以外）---
	// ピッキング対象（ワールド空間のバウンディング球）を毎フレーム組み立てる
	std::vector<DebugCamera::PickTarget> pickTargets;
	AppendPickTargets(pickTargets);

	// ImGuiがマウスを使用中はデバッグカメラのマウス操作を無視する
	bool blockMouse = false;
#ifdef USE_IMGUI
	blockMouse = ImGui::GetIO().WantCaptureMouse;
#endif
	debugCamera_.Update(
		pickTargets,
		GetRenderAreaX(), GetRenderAreaY(), GetRenderAreaWidth(), GetRenderAreaHeight(),
		camera_.GetProjectionMatrix(GetAspectRatio()), blockMouse);
#endif  // !NDEBUG

	// 天球（カメラ追従ON時は中心がカメラ位置へ追従する）
	skydome_.Update(CalcViewMatrix());
}

#ifndef NDEBUG
Matrix4x4 DemoSceneBase::CalcViewMatrix() const {
	// デバッグカメラ有効時は通常カメラのビューを上書きする
	return debugCamera_.IsEnabled() ? debugCamera_.GetViewMatrix() : camera_.GetViewMatrix();
}
#endif

void DemoSceneBase::OnDraw() {
	// --- 天球を最初に描画（背景。カリング無効PSOで描き、標準PSOへ戻る）---
	skydome_.Draw();

	// --- シーン固有オブジェクトの描画 ---
	OnDrawObjects();

	// --- 選択オブジェクトの回転軸ギズモ（最後に描画。深度無効で他オブジェクトに隠れない）---
	axisGizmo_.Draw();
}

#ifdef USE_IMGUI
void DemoSceneBase::OnDrawImGui() {
	// --- 3Dオブジェクト（シーン固有＋天球）---
	ImGui::Begin("3D Objects");
	padController_.DrawImGui();
	axisGizmo_.DrawImGui();
	ImGui::Separator();
	OnDrawObjectsImGui();
	skydome_.DrawImGui();
	ImGui::End();

	// --- シーン固有の追加ウィンドウ（GameSceneの"2D Objects"など）---
	OnDrawExtraImGui();

	// --- カメラ（変換・立体視・視線追跡）---
	ImGui::Begin("Camera");

	Transform3D& cameraTransform = camera_.GetTransform();
	ImGui::DragFloat3("Camera scale", &cameraTransform.scale.x, 0.01f);
	ImGui::DragFloat3("Camera rotate", &cameraTransform.rotate.x, 0.01f);
	ImGui::DragFloat3("Camera translate", &cameraTransform.translate.x, 0.01f);

	ImGui::Separator();

	// 立体視の共有パラメータと頭連動オフアクシスの調整
	stereoCamera_.DrawImGuiSection();

	ImGui::End();

	// --- 平行光源 ---
	ImGui::Begin("Directional Light");

	bool directionalEnabled = directionalLight_.enabled != 0;
	if (ImGui::Checkbox("Enable", &directionalEnabled)) {
		directionalLight_.enabled = directionalEnabled ? 1 : 0;
	}
	ImGui::ColorEdit4("Color", &directionalLight_.color.x);
	ImGui::DragFloat3("Direction", &directionalLight_.direction.x, 0.01f);
	ImGui::DragFloat("Intensity", &directionalLight_.intensity, 0.01f, 0.0f, 10.0f);

	ImGui::End();

	// --- 点光源 ---
	ImGui::Begin("Point Lights");

	// 全灯まとめて切り替え
	if (ImGui::Button("All ON")) {
		for (PointLight& pointLight : pointLights_.lights) {
			pointLight.enabled = 1;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("All OFF")) {
		for (PointLight& pointLight : pointLights_.lights) {
			pointLight.enabled = 0;
		}
	}

	ImGui::Separator();

	for (uint32_t i = 0; i < kMaxPointLightCount; ++i) {
		PointLight& pointLight = pointLights_.lights[i];

		// ラベルに現在の状態を表示する（IDはインデックス由来なのでラベルが変わっても開閉状態は保たれる）
		if (ImGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(i)),
			"Light %u (%s)", i, pointLight.enabled != 0 ? "ON" : "OFF")) {

			bool enabled = pointLight.enabled != 0;
			if (ImGui::Checkbox("Enable", &enabled)) {
				pointLight.enabled = enabled ? 1 : 0;
			}
			ImGui::ColorEdit4("Color", &pointLight.color.x);
			ImGui::DragFloat3("Position", &pointLight.position.x, 0.01f);
			ImGui::DragFloat("Intensity", &pointLight.intensity, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("Radius", &pointLight.radius, 0.05f, 0.01f, 100.0f);
			ImGui::DragFloat("Decay", &pointLight.decay, 0.01f, 0.05f, 8.0f);

			ImGui::TreePop();
		}
	}

	ImGui::End();

#ifndef NDEBUG
	// デバッグカメラの状態表示・調整（Release以外）
	debugCamera_.DrawImGui();
#endif

	ImGui::Begin("Sound");

	// ボタンでもSpaceキーでも再生できる（再生中は頭から鳴らし直す）
	if (ImGui::Button("Play (Alarm01)")) {
		Audio::GetInstance()->Play(soundHandle_);
	}

	// 音量を増減する
	if (ImGui::SliderFloat("Volume", &soundVolume_, 0.0f, 1.0f)) {
		Audio::GetInstance()->SetVolume(soundHandle_, soundVolume_);
	}

	ImGui::End();

	// --- 視錐台カリングの判定結果表示 ---
	ImGui::Begin("Frustum Culling");
	OnDrawCullingImGui();
	ImGui::End();
}

const char* DemoSceneBase::VisibilityText(FrustumVisibility visibility) {
	switch (visibility) {
	case FrustumVisibility::Inside:    return "Inside";
	case FrustumVisibility::Intersect: return "Intersect";
	case FrustumVisibility::Outside:   return "Outside";
	}
	return "Unknown";
}

void DemoSceneBase::DrawLightingModeCombo(Material& material) {
	static const char* kModeItems[] = { "None", "Lambert", "Half Lambert" };
	int mode = static_cast<int>(material.lightingMode);
	if (ImGui::Combo("Lighting", &mode, kModeItems, IM_ARRAYSIZE(kModeItems))) {
		material.lightingMode = static_cast<LightingMode>(mode);
	}
}

void DemoSceneBase::DrawMaterialEditor(Object3D& object) {
	const uint32_t materialCount = object.GetMaterialCount();
	const bool useTreeNode = materialCount > 1;

	for (uint32_t i = 0; i < materialCount; ++i) {
		if (useTreeNode) {
			// ノード名はmtlのマテリアル名（同名でも区別できるようインデックスをIDにする）。
			// Framedで背景付きのヘッダーバーにし、開閉できる項目だと分かりやすくする
			const std::string& materialName = object.GetMesh()->GetSubMesh(i).materialName;
			if (!ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(i)),
					ImGuiTreeNodeFlags_Framed,
					"Material %u: %s", i,
					materialName.empty() ? "(nomtl)" : materialName.c_str())) {
				continue;
			}
		}
		ImGui::PushID(static_cast<int>(i));

		Material& material = object.GetMaterial(i);
		ImGui::ColorEdit4("Color", &material.color.x);
		DrawLightingModeCombo(material);

		Transform3D& uvTransform = object.GetUVTransform(i);
		ImGui::DragFloat2("UVTranslate", &uvTransform.translate.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2("UVScale", &uvTransform.scale.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle("UVRotate", &uvTransform.rotate.z);

		ImGui::PopID();
		if (useTreeNode) {
			ImGui::TreePop();
		}
	}
}

void DemoSceneBase::DrawModelTextureCombo(Object3D& object, int& textureIndex) {
	// 先頭にmtl由来（既定）の項目を足した選択肢（一括上書き後もここで0を選べば戻せる）
	static const char* kModelTextureItems[] = { "MTL (default)", "uvChecker", "monsterBall" };
	if (ImGui::Combo("Texture", &textureIndex, kModelTextureItems, IM_ARRAYSIZE(kModelTextureItems))) {
		if (textureIndex == 0) {
			object.ClearTextureOverride();
		} else {
			object.SetTextureHandle(textureHandles_[textureIndex - 1]);
		}
	}
}

void DemoSceneBase::DrawSubMeshInfo(const Mesh& mesh) {
	ImGui::Text("SubMeshes: %u", mesh.GetSubMeshCount());
	for (uint32_t i = 0; i < mesh.GetSubMeshCount(); ++i) {
		const Mesh::SubMesh& subMesh = mesh.GetSubMesh(i);
		ImGui::BulletText("%s / %s : %u verts (%s)",
			subMesh.name.empty() ? "(noname)" : subMesh.name.c_str(),
			subMesh.materialName.empty() ? "(nomtl)" : subMesh.materialName.c_str(),
			subMesh.vertexCount,
			subMesh.textureFilePath.empty() ? "white" : subMesh.textureFilePath.c_str());
	}
}
#endif
