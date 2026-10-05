#include "Sandbox/Object/Skydome.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Rendering/RenderContext.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void Skydome::Initialize() {
	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

	// --- モデル読み込み（半径1のユニット球）---
	mesh_.CreateFromObj(device, "resources", "skydome.obj");

	// --- テクスチャ ---
	textureHandle_ = TextureManager::GetInstance()->Load("resources/sky_sphere.png");

	// --- マテリアル（ライティング無効でテクスチャをそのまま表示）---
	material_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	material_.lightingMode = LightingMode::kNone;
	material_.uvTransform = MakeIdentity4x4();
	materialCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- Transform ---
	transformCB_.Create(device, DirectXCore::kFramesInFlight);
}

void Skydome::Update(const Matrix4x4& centerView) {
	// カメラ追従ON時は中心をカメラのワールド位置へ合わせ、どこへ動いても境界が見えないようにする。
	// ビュー行列の逆行列がカメラのワールド行列なので、その原点を変換して位置を得る。
	Vector3 center = position_;
	if (followCamera_) {
		Matrix4x4 cameraWorld = Inverse(centerView);
		Vector3 origin{ 0.0f, 0.0f, 0.0f };
		center = Transform(origin, cameraWorld);
	}

	Matrix4x4 world = MakeAffineMatrix({ scale_, scale_, scale_ }, { 0.0f, 0.0f, 0.0f }, center);
	TransformationMatrix transformData{ world };

	uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	transformCB_.Write(frameIndex, transformData);
	materialCB_.Write(frameIndex, material_);
}

void Skydome::Draw() {
	ID3D12GraphicsCommandList* commandList = RenderContext::GetInstance()->GetCommandList();
	PipelineManager* pipelineManager = PipelineManager::GetInstance();

	// カリング無効PSOに切り替える（RootSignature・ビュー射影・光源はシーンで設定済み）
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kNoCull);

	uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	commandList->SetGraphicsRootConstantBufferView(
		PipelineManager::kRootMaterial, materialCB_.GetGPUAddress(frameIndex));
	commandList->SetGraphicsRootConstantBufferView(
		PipelineManager::kRootWorldTransform, transformCB_.GetGPUAddress(frameIndex));
	commandList->SetGraphicsRootDescriptorTable(
		PipelineManager::kRootTexture, TextureManager::GetInstance()->GetSrvHandleGPU(textureHandle_));

	mesh_.Draw(commandList);

	// 後に続くオブジェクトのために標準PSO（裏面カリング）へ戻す
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kStandard);
}

#ifdef USE_IMGUI
void Skydome::DrawImGui() {
	if (ImGui::TreeNode("Skydome")) {
		ImGui::PushID("Skydome");

		ImGui::Checkbox("Follow Camera", &followCamera_);
		ImGui::DragFloat("Scale", &scale_, 0.1f, 1.0f, 90.0f);
		// 原点固定時のみ中心位置を調整できる（追従中はカメラ位置で上書きされる）
		if (!followCamera_) {
			ImGui::DragFloat3("Position", &position_.x, 0.1f);
		}

		ImGui::PopID();
		ImGui::TreePop();
	}
}
#endif
