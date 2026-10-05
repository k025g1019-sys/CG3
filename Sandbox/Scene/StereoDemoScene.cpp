#include "Sandbox/Scene/StereoDemoScene.h"

#include <string>
#include <vector>

#include "Engine/Core/DirectXCore.h"
#include "Engine/Culling/FrustumCulling.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Math/Matrix4x4.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void StereoDemoScene::OnInitialize() {
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
	skydome_.Initialize();

	// --- 選択立方体の回転軸ギズモ ---
	axisGizmo_.Initialize();

	// --- 平行光源（立方体の面の向きが分かるよう、斜め下向きにする。点光源は使わない）---
	directionalLight_.direction = { 0.4f, -1.0f, 0.6f };

	// --- カメラ初期位置（被写体までの距離を収束距離の既定10に合わせる）---
	camera_.GetTransform().rotate = { 0.4f, -0.78f, 0.0f };
	camera_.GetTransform().translate = { 12.8f, 10.0f, -12.8f };
}

void StereoDemoScene::OnUpdate() {
#ifndef NDEBUG
	// --- デバッグカメラ更新（Release以外）---
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
	debugCamera_.Update(
		pickTargets,
		GetRenderAreaX(), GetRenderAreaY(), GetRenderAreaWidth(), GetRenderAreaHeight(),
		camera_.GetProjectionMatrix(GetAspectRatio()), blockMouse);
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

	// --- 各立方体の更新（ワールド行列・定数バッファ書き込み）---
	for (std::unique_ptr<Object3D>& cube : cubes_) {
		cube->Update();
	}

	// 天球（カメラ追従ON時は中心がカメラ位置へ追従する）
	skydome_.Update(CalcViewMatrix());
}

#ifndef NDEBUG
Matrix4x4 StereoDemoScene::CalcViewMatrix() const {
	// デバッグカメラ有効時は通常カメラのビューを上書きする
	return debugCamera_.IsEnabled() ? debugCamera_.GetViewMatrix() : camera_.GetViewMatrix();
}
#endif

void StereoDemoScene::AddCube(const Vector3& position, float rotateY) {
	std::unique_ptr<Object3D> cube = std::make_unique<Object3D>();
	cube->Initialize(&cubeMesh_, cubeTextureHandle_);
	cube->GetTransform().translate = position;
	cube->GetTransform().rotate.y = rotateY;
	cubes_.push_back(std::move(cube));
}

#ifdef USE_IMGUI
void StereoDemoScene::OnDrawImGui() {
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

	bool directionalEnabled = directionalLight_.enabled != 0;
	if (ImGui::Checkbox("Enable", &directionalEnabled)) {
		directionalLight_.enabled = directionalEnabled ? 1 : 0;
	}
	ImGui::ColorEdit4("Color", &directionalLight_.color.x);
	ImGui::DragFloat3("Direction", &directionalLight_.direction.x, 0.01f);
	ImGui::DragFloat("Intensity", &directionalLight_.intensity, 0.01f, 0.0f, 10.0f);

	ImGui::End();

#ifndef NDEBUG
	// デバッグカメラの状態表示・調整（Release以外）
	debugCamera_.DrawImGui();
#endif
}
#endif

void StereoDemoScene::OnDraw() {
	// --- 天球を最初に描画（背景。カリング無効PSOで描き、標準PSOへ戻る）---
	skydome_.Draw();

	for (const std::unique_ptr<Object3D>& cube : cubes_) {
		cube->Draw();
	}

	// --- 選択立方体の回転軸ギズモ（最後に描画。深度無効で他オブジェクトに隠れない）---
	axisGizmo_.Draw();
}
