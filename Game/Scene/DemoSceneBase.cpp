#include "Game/Scene/DemoSceneBase.h"

#include "Engine/Audio/Audio.h"
#include "Engine/Core/DirectXCore.h"
#include "Engine/Core/WinApp.h"
#include "Engine/Graphics/PipelineManager.h"
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

void DemoSceneBase::Initialize() {
	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

	// --- シーン共通のテクスチャ ---
	textureHandles_[0] = TextureManager::GetInstance()->Load("resources/uvChecker.png");
	textureHandles_[1] = TextureManager::GetInstance()->Load("resources/monsterBall.png");

	// --- 天球（背景。ライティング無効・カリング無効PSO）---
	skydome_.Initialize(device);

	// --- 平行光源 ---
	lightCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- 点光源 ---
	pointLights_.lights[0].enabled = 0;
	pointLightCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- 立体視：視点ごとのビュー射影CBuffer ---
	stereoCamera_.Initialize(device);

	// --- カメラ初期位置（派生シーンがOnInitializeで上書きしてもよい）---
	camera_.GetTransform().rotate = { 0.04f, 0.0f, 0.0f };
	camera_.GetTransform().translate = { 0.0f, 1.7f, -10.0f };

	// --- サウンド読み込み ---
	soundHandle_ = Audio::GetInstance()->LoadWave("resources/Alarm01.wav");
	Audio::GetInstance()->SetVolume(soundHandle_, soundVolume_);

	// --- シーン固有のリソース生成 ---
	OnInitialize(device);
}

void DemoSceneBase::Update() {
	// スペースキーを押した瞬間にサウンド再生（Enterはデバッグカメラの有効・無効切り替えに割り当て）
	if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
		Audio::GetInstance()->Play(soundHandle_);
	}

	// ゲームの描画先矩形に合わせて投影アスペクトを決める（リサイズやドッキングの
	// レイアウト変更に追従し、物体が伸び縮みして見えるのを防ぐ）。
	// 未設定（サイズ0）の間はウィンドウ全体を使う。
	const float width = float(WinApp::GetInstance()->GetClientWidth());
	const float height = float(WinApp::GetInstance()->GetClientHeight());
	const bool hasRenderArea = (renderAreaWidth_ > 0.0f && renderAreaHeight_ > 0.0f);
	const float viewWidth = hasRenderArea ? renderAreaWidth_ : width;
	const float viewHeight = hasRenderArea ? renderAreaHeight_ : height;

	Matrix4x4 projection = camera_.GetProjectionMatrix(viewWidth / viewHeight);
	Matrix4x4 view = camera_.GetViewMatrix();

#ifndef NDEBUG
	// --- デバッグカメラ更新（Debugビルドのみ。Releaseでは丸ごと除外される）---
	// ピッキング対象（ワールド空間のバウンディング球）を毎フレーム組み立てる
	std::vector<DebugCamera::PickTarget> pickTargets;
	AppendPickTargets(pickTargets);

	// ImGuiがマウスを使用中はデバッグカメラのマウス操作を無視する
	bool blockMouse = false;
#ifdef USE_IMGUI
	blockMouse = ImGui::GetIO().WantCaptureMouse;
#endif
	const float viewX = hasRenderArea ? renderAreaX_ : 0.0f;
	const float viewY = hasRenderArea ? renderAreaY_ : 0.0f;
	debugCamera_.Update(pickTargets, viewX, viewY, viewWidth, viewHeight, projection, blockMouse);

	// デバッグカメラ有効時は通常カメラのビューを上書きする
	if (debugCamera_.IsEnabled()) {
		view = debugCamera_.GetViewMatrix();
	}
#endif  // !NDEBUG

	// --- シーン固有オブジェクトの更新（ワールド行列・定数バッファ書き込み・カリング判定）---
	// カリングは中心カメラの視錐台で判定する（視点間のずれは眼間距離程度で無視できる）。
	Frustum3D frustum = MakeFrustumFromViewProjection(view * projection);
	OnUpdate(frustum, viewWidth, viewHeight);

	// 天球（カメラ追従ON時は中心がカメラ位置へ追従する）
	skydome_.Update(view);

	// 平行光源・点光源
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	lightCB_.Write(frameIndex, light_);
	pointLightCB_.Write(frameIndex, pointLights_);

	// --- 立体視：中心カメラから各視点（眼）のビュー射影を更新する ---
	stereoCamera_.Update(view, projection);
}

void DemoSceneBase::Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) {
	uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();

	// --- 共通設定（Viewport/Scissor/RenderTarget/DescriptorHeapは
	//     DirectXCore::BeginFrameまたはStereoRenderer::BeginViewで設定済み）---
	commandList->SetGraphicsRootSignature(PipelineManager::GetInstance()->GetRootSignature());

	// この視点のビュー射影をVS(b1)へバインド（以降の3D描画で共有。スプライトのみ自前の正射影へ差し替える）
	commandList->SetGraphicsRootConstantBufferView(
		4, stereoCamera_.GetViewProjectionAddress(frameIndex, viewIndex));

	// 点光源のCBufferはシーン共通（PS b2。天球を含む以降の全描画で共有される）
	commandList->SetGraphicsRootConstantBufferView(5, pointLightCB_.GetGPUAddress(frameIndex));

	// --- 天球を最初に描画（背景。カリング無効PSOに切り替わる）---
	skydome_.Draw(commandList, lightCB_.GetGPUAddress(frameIndex));

	// --- 以降は標準PSO（裏面カリング）で描画 ---
	commandList->SetPipelineState(PipelineManager::GetInstance()->Get(PipelineManager::Pipeline::kStandard));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 平行光源のCBufferはシーン共通（各オブジェクトのマテリアル・Transformは各自が設定する）
	commandList->SetGraphicsRootConstantBufferView(2, lightCB_.GetGPUAddress(frameIndex));

	// --- シーン固有オブジェクトの描画 ---
	OnDraw(commandList);
}

#ifdef USE_IMGUI
void DemoSceneBase::DrawImGui() {
	// --- 3Dオブジェクト（シーン固有＋天球）---
	ImGui::Begin("3D Objects");
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

	bool directionalEnabled = light_.enabled != 0;
	if (ImGui::Checkbox("Enable", &directionalEnabled)) {
		light_.enabled = directionalEnabled ? 1 : 0;
	}
	ImGui::ColorEdit4("Color", &light_.color.x);
	ImGui::DragFloat3("Direction", &light_.direction.x, 0.01f);
	ImGui::DragFloat("Intensity", &light_.intensity, 0.01f, 0.0f, 10.0f);

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
	// デバッグカメラの状態表示・調整（Debugビルドのみ）
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
