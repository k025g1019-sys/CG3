#include "Sandbox/Scene/StereoDemoScene.h"

#include <string>
#include <vector>

#include "Engine/Core/DirectXCore.h"
#include "Engine/Core/WinApp.h"
#include "Engine/Culling/FrustumCulling.h"
#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Math/Matrix4x4.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void StereoDemoScene::Initialize() {
	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

	// --- テクスチャ ---
	cubeTextureHandle_ = TextureManager::GetInstance()->Load("resources/tilemap_ground_01.png");
	//cubeTextureHandle_ = TextureManager::GetInstance()->Load("resources/tilemap_ground_01_1tileonly.png");

	// --- 立方体（共有メッシュ＋初期配置）---
	cubeMesh_.CreateCube(device);
	// 後列
	AddCube({ -4.0f, 0.0f, 4.0f }, 0.0f);
	AddCube({ -4.0f, 1.0f, 4.0f }, 0.0f);
	AddCube({ -4.0f, 2.0f, 4.0f }, 0.0f);
	AddCube({ -4.0f, 3.0f, 4.0f }, 0.0f);
	AddCube({ -2.0f, 0.0f, 4.0f }, 0.0f);
	AddCube({ 0.0f, 0.0f, 4.0f }, 0.0f);
	AddCube({ 0.0f, 1.0f, 4.0f }, 0.0f);
	AddCube({ 0.0f, 2.0f, 4.0f }, 0.0f);
	AddCube({ 2.0f, 0.0f, 4.0f }, 0.0f);
	// 中後列
	AddCube({ -4.0f, 0.0f, 2.0f }, 0.0f);
	AddCube({ -2.0f, 0.0f, 2.0f }, 0.0f);
	AddCube({ 0.0f, 0.0f, 2.0f }, 0.0f);
	AddCube({ 2.0f, 0.0f, 2.0f }, 0.0f);
	// 中列
	AddCube({ -4.0f, 0.0f, 0.0f }, 0.0f);
	AddCube({ -2.0f, 0.0f, 0.0f }, 0.0f);
	AddCube({ 0.0f, 0.0f, 0.0f }, 0.0f);
	AddCube({ 0.0f, 2.0f, 0.0f }, 0.0f);
	AddCube({ 2.0f, 0.0f, 0.0f }, 0.0f);
	// 前列
	AddCube({ -4.0f, 0.0f, -2.0f }, -0.0f);
	AddCube({ -2.0f, 0.0f, -2.0f }, -0.0f);
	AddCube({ -2.0f, 2.0f, -2.0f }, -0.0f);
	AddCube({ 0.0f, 0.0f, -2.0f }, -0.0f);
	AddCube({ 0.0f, 1.0f, -2.0f }, -0.0f);
	AddCube({ 2.0f, 0.0f, -2.0f }, -0.0f);

	// --- 天球（背景。ライティング無効・カリング無効PSO）---
	skydome_.Initialize(device);

	// --- 選択立方体の回転軸ギズモ ---
	axisGizmo_.Initialize(device);

	// --- 平行光源 ---
	lightCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- 点光源（全灯無効のまま。共通ルートシグネチャが要求するためCBufferだけ用意する）---
	pointLightCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- 立体視：視点ごとのビュー射影CBuffer ---
	stereoCamera_.Initialize(device);

	// --- カメラ初期位置（被写体までの距離を収束距離の既定10に合わせる）---
	camera_.GetTransform().rotate = { 0.4f, -0.78f, 0.0f };
	camera_.GetTransform().translate = { 12.8f, 10.0f, -12.8f };
}

void StereoDemoScene::Update() {
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
	for (const std::unique_ptr<Object3D>& cube : cubes_) {
		Sphere sphere = cube->CalcWorldBoundingSphere();
		pickTargets.push_back({ sphere.center, sphere.radius });
	}

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

	// --- パッドで選択立方体を操作（対象リストは毎フレーム組み立て、追加・削除に追従する）---
	std::vector<PadObjectController::Target> padTargets;
	padTargets.reserve(cubes_.size());
	for (int i = 0; i < int(cubes_.size()); ++i) {
		padTargets.push_back({ "Cube " + std::to_string(i), cubes_[size_t(i)].get() });
	}
	padController_.SetTargets(std::move(padTargets));
	padController_.Update();

	// 選択立方体の回転軸ギズモを追従させる（定数バッファ書き込みは毎フレームここだけ）
	axisGizmo_.Update(padController_.GetSelectedObject());

	// --- 各立方体の更新（ワールド行列・定数バッファ書き込み・視錐台カリング）---
	// カリングは中心カメラの視錐台で判定する（視点間のずれは眼間距離程度で無視できる）。
	Frustum3D frustum = MakeFrustumFromViewProjection(view * projection);
	for (std::unique_ptr<Object3D>& cube : cubes_) {
		cube->Update(frustum);
	}

	// 天球（カメラ追従ON時は中心がカメラ位置へ追従する）
	skydome_.Update(view);

	// 平行光源・点光源
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	lightCB_.Write(frameIndex, light_);
	pointLightCB_.Write(frameIndex, pointLights_);

	// --- 立体視：中心カメラから各視点（眼）のビュー射影を更新する ---
	stereoCamera_.Update(view, projection);
}

void StereoDemoScene::AddCube(const Vector3& position, float rotateY) {
	std::unique_ptr<Object3D> cube = std::make_unique<Object3D>();
	cube->Initialize(DirectXCore::GetInstance()->GetDevice(), &cubeMesh_, cubeTextureHandle_);
	cube->GetTransform().translate = position;
	cube->GetTransform().rotate.y = rotateY;
	cubes_.push_back(std::move(cube));
}

#ifdef USE_IMGUI
void StereoDemoScene::DrawImGui() {
	ImGui::Begin("3D Objects");

	padController_.DrawImGui();
	axisGizmo_.DrawImGui();
	ImGui::Separator();

	// ----Cubes----
	ImGui::Text("Cubes: %d", int(cubes_.size()));
	ImGui::SameLine();
	if (ImGui::Button("Add")) {
		// 既存の立方体と重ならないよう、最後の立方体の右隣に追加する
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		if (!cubes_.empty()) {
			position = cubes_.back()->GetTransform().translate;
			position.x += 1.5f;
		}
		AddCube(position, 0.0f);
	}

	// 各立方体の個別編集。削除はループ中のvector変更を避けるため、印だけ付けてループ後に行う
	int removeIndex = -1;
	for (int i = 0; i < int(cubes_.size()); ++i) {
		Object3D& cube = *cubes_[i];

		// IDはインデックス由来（ラベルが変わっても開閉状態は保たれる）
		if (ImGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(i)), "Cube %d", i)) {

			Transform3D& transform = cube.GetTransform();
			ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
			ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
			ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
			ImGui::Separator();

			ImGui::ColorEdit4("Color", &cube.GetMaterial().color.x);
			// このシーンは従来どおりON/OFFのみ（ON=Half Lambert）
			bool lighting = cube.GetMaterial().lightingMode != LightingMode::kNone;
			if (ImGui::Checkbox("Enable Lighting", &lighting)) {
				cube.GetMaterial().lightingMode =
					lighting ? LightingMode::kHalfLambert : LightingMode::kNone;
			}

			if (ImGui::Button("Remove")) {
				removeIndex = i;
			}

			ImGui::TreePop();
		}
	}
	if (removeIndex >= 0) {
		// 実行中のフレームが削除対象の定数バッファを参照している可能性があるため、
		// FenceでGPU完了待ちをしてから解放する（球の分割数変更と同じ扱い）
		DirectXCore::GetInstance()->WaitForGPU();
		cubes_.erase(cubes_.begin() + removeIndex);
	}

	ImGui::Separator();

	// ----Skydome----
	skydome_.DrawImGui();

	ImGui::End();

	// --- カメラ（変換・立体視・視線追跡）---
	ImGui::Begin("Camera");

	Transform3D& cameraTransform = camera_.GetTransform();
	ImGui::DragFloat3("Camera rotate", &cameraTransform.rotate.x, 0.01f);
	ImGui::DragFloat3("Camera translate", &cameraTransform.translate.x, 0.01f);

	ImGui::Separator();

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

#ifndef NDEBUG
	// デバッグカメラの状態表示・調整（Debugビルドのみ）
	debugCamera_.DrawImGui();
#endif
}
#endif

void StereoDemoScene::Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) {
	uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();

	// --- 共通設定（Viewport/Scissor/RenderTarget/DescriptorHeapは
	//     DirectXCore::BeginFrameまたはStereoRenderer::BeginViewで設定済み）---
	commandList->SetGraphicsRootSignature(PipelineManager::GetInstance()->GetRootSignature());

	// この視点のビュー射影をVS(b1)へバインド（以降の3D描画で共有する）
	commandList->SetGraphicsRootConstantBufferView(
		4, stereoCamera_.GetViewProjectionAddress(frameIndex, viewIndex));

	// 点光源のCBufferはシーン共通（PS b2。全灯無効だがルートシグネチャが要求する）
	commandList->SetGraphicsRootConstantBufferView(5, pointLightCB_.GetGPUAddress(frameIndex));

	// --- 天球を最初に描画（背景。カリング無効PSOに切り替わる）---
	skydome_.Draw(commandList, lightCB_.GetGPUAddress(frameIndex));

	// --- 以降は標準PSO（裏面カリング）で描画 ---
	commandList->SetPipelineState(PipelineManager::GetInstance()->Get(PipelineManager::Pipeline::kStandard));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 平行光源のCBufferはシーン共通（各立方体のマテリアル・Transformは各自が設定する）
	commandList->SetGraphicsRootConstantBufferView(2, lightCB_.GetGPUAddress(frameIndex));

	for (const std::unique_ptr<Object3D>& cube : cubes_) {
		cube->Draw(commandList);
	}

	// --- 選択立方体の回転軸ギズモ（最後に描画。深度無効で他オブジェクトに隠れない）---
	axisGizmo_.Draw(commandList, stereoCamera_.GetViewProjectionAddress(frameIndex, viewIndex));
}
