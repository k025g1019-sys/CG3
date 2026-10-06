#include "Engine/Rendering/ParticleSystem.h"

#include <cassert>

#include "Engine/Core/DirectXCore.h"
#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Math/Matrix4x4.h"
#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/MeshManager.h"
#include "Engine/Rendering/RenderContext.h"

namespace Engine {

void ParticleSystem::Initialize(const std::string& objFilePath, uint32_t maxInstanceCount) {
	assert(maxInstanceCount > 0);

	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

	// --- メッシュ（同じOBJは1回だけ読み込まれ、Object3Dとも共有される）---
	mesh_ = MeshManager::GetInstance()->Load(objFilePath);

	// --- テクスチャ（最初のサブメッシュのmtl由来。map_Kdが無ければ白）---
	TextureManager* textureManager = TextureManager::GetInstance();
	textureHandle_ = textureManager->GetWhiteTexture();
	if (mesh_->GetSubMeshCount() > 0 && !mesh_->GetSubMesh(0).textureFilePath.empty()) {
		textureHandle_ = textureManager->Load(mesh_->GetSubMesh(0).textureFilePath);
	}

	// --- マテリアル（全インスタンス共通。パーティクルはライティングしない）---
	material_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	material_.lightingMode = LightingMode::kNone;
	material_.uvTransform = MakeIdentity4x4();
	materialCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- インスタンスごとのTransform（最大数ぶん。最初は全部を描く）---
	transforms_.assign(
		maxInstanceCount,
		Transform3D{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } });
	instanceCount_ = maxInstanceCount;

	// --- Instancing用にTransformationMatrixを最大数ぶん格納できるResourceを作る ---
	// （フレームインフライト数ぶんのスロットを持ち、スロットごとにSRVを作る）
	instancingBuffer_.Create(device, maxInstanceCount, DirectXCore::kFramesInFlight);

	// 単位行列を書きこんでおく
	for (uint32_t slot = 0; slot < DirectXCore::kFramesInFlight; ++slot) {
		TransformationMatrix* instancingData = instancingBuffer_.GetData(slot);
		for (uint32_t index = 0; index < maxInstanceCount; ++index) {
			instancingData[index].World = MakeIdentity4x4();
		}
	}
}

void ParticleSystem::Update() {
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();

	// --- 各インスタンスのワールド行列を、このフレームのスロットへ書き込む ---
	// （ビュー射影は視点ごとの共有CBuffer（VSのb1）で渡すため、ここではワールド行列だけを計算する）
	TransformationMatrix* instancingData = instancingBuffer_.GetData(frameIndex);
	for (uint32_t index = 0; index < instanceCount_; ++index) {
		const Transform3D& transform = transforms_[index];
		instancingData[index].World =
			MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	}

	// --- マテリアル（UV変換行列を組み立てて書き込む）---
	Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransform_.scale);
	uvTransformMatrix *= MakeRotateZMatrix(uvTransform_.rotate.z);
	uvTransformMatrix *= MakeTranslateMatrix(uvTransform_.translate);
	material_.uvTransform = uvTransformMatrix;
	materialCB_.Write(frameIndex, material_);
}

void ParticleSystem::Draw() const {
	if (instanceCount_ == 0) {
		return;
	}
	// 確保したResourceの数を超えて描くと、GPUが範囲外を読んでしまう
	assert(instanceCount_ <= instancingBuffer_.GetElementCount());

	RenderContext* context = RenderContext::GetInstance();
	ID3D12GraphicsCommandList* commandList = context->GetCommandList();
	PipelineManager* pipelineManager = PipelineManager::GetInstance();
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();

	// --- パーティクル用のRootSignature・PSOへ切り替える ---
	// （RootSignatureを変えると設定済みのCBV等は無効になるため、使うものはすべて設定し直す）
	commandList->SetGraphicsRootSignature(pipelineManager->GetParticleRootSignature());
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kParticle);

	commandList->SetGraphicsRootConstantBufferView(
		PipelineManager::kParticleRootMaterial, materialCB_.GetGPUAddress(frameIndex));
	// instancing用のDataを読むためにStructuredBufferのSRVを設定する
	commandList->SetGraphicsRootDescriptorTable(
		PipelineManager::kParticleRootInstancing, instancingBuffer_.GetSrvHandleGPU(frameIndex));
	commandList->SetGraphicsRootDescriptorTable(
		PipelineManager::kParticleRootTexture, TextureManager::GetInstance()->GetSrvHandleGPU(textureHandle_));
	// この視点のビュー射影（立体視では視点ごとに違う）
	commandList->SetGraphicsRootConstantBufferView(
		PipelineManager::kParticleRootViewProjection, context->GetViewProjectionAddress());

	// --- 描画！インスタンス数ぶんを1回の描画命令でまとめて描く ---
	mesh_->Draw(commandList, instanceCount_);

	// --- 後に続く3D描画のために、標準のRootSignature・CBV・PSOへ戻す ---
	context->BindStandardState(commandList);
}

void ParticleSystem::SetInstanceCount(uint32_t instanceCount) {
	// インスタンス数が確保したResourceの数を超えないようにする（少ない分には問題ない）
	assert(instanceCount <= GetMaxInstanceCount());
	instanceCount_ = (instanceCount <= GetMaxInstanceCount()) ? instanceCount : GetMaxInstanceCount();
}

Transform3D& ParticleSystem::GetTransform(uint32_t index) {
	assert(index < transforms_.size());
	return transforms_[index];
}

const Transform3D& ParticleSystem::GetTransform(uint32_t index) const {
	assert(index < transforms_.size());
	return transforms_[index];
}

void ParticleSystem::SetTexture(const std::string& textureFilePath) {
	SetTextureHandle(TextureManager::GetInstance()->Load(textureFilePath));
}

} // namespace Engine
