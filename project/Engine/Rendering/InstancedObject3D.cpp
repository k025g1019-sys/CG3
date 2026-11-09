#include "InstancedObject3D.h"

#include <cassert>

#include "../Core/DirectXCore.h"
#include "../Graphics/PipelineManager.h"
#include "../Graphics/TextureManager.h"
#include "Mesh.h"
#include "MeshManager.h"
#include "RenderContext.h"

namespace Engine {

void InstancedObject3D::Initialize(const std::string& objFilePath, uint32_t maxInstanceCount) {
	assert(maxInstanceCount > 0);

	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

	// --- メッシュ（同じOBJは1回だけ読み込まれ、Object3Dやパーティクルとも共有される）---
	mesh_ = MeshManager::GetInstance()->Load(objFilePath);

	// --- テクスチャ（最初のサブメッシュのmtl由来。map_Kdが無ければ白）---
	TextureManager* textureManager = TextureManager::GetInstance();
	textureHandle_ = textureManager->GetWhiteTexture();
	if (mesh_->GetSubMeshCount() > 0 && !mesh_->GetSubMesh(0).textureFilePath.empty()) {
		textureHandle_ = textureManager->Load(mesh_->GetSubMesh(0).textureFilePath);
	}

	// --- マテリアル（全インスタンス共通。ライティングはしない）---
	material_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	material_.lightingMode = LightingMode::kNone;
	material_.uvTransform = MakeIdentity4x4();
	materialCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- インスタンスごとのTransform（最初は全部原点。位置は使う側が決める）---
	transforms_.assign(maxInstanceCount, Transform3D{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } });
	instanceCount_ = maxInstanceCount;

	// --- Instancing用にInstanceForGPUを最大数ぶん格納できるResourceを作る ---
	// （フレームインフライト数ぶんのスロットを持ち、スロットごとにSRVを作る）
	instancingBuffer_.Create(device, maxInstanceCount, DirectXCore::kFramesInFlight);

	// 各フレームのインスタンシングバッファを初期化（色はインスタンスごとには変えないので白のまま）
	for (uint32_t slot = 0; slot < DirectXCore::kFramesInFlight; ++slot) {
		InstanceForGPU* instancingData = instancingBuffer_.GetData(slot);

		for (uint32_t index = 0; index < maxInstanceCount; ++index) {
			instancingData[index].World = MakeIdentity4x4();
			instancingData[index].color = { 1.0f, 1.0f, 1.0f, 1.0f };
		}
	}
}

void InstancedObject3D::Update() {
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();

	// --- 各インスタンスのワールド行列を、このフレームのスロットへ書き込む ---
	// （ビュー射影は視点ごとの共有CBuffer（VSのb1）で渡すため、ここではワールド行列だけを計算する）
	InstanceForGPU* instancingData = instancingBuffer_.GetData(frameIndex);
	for (uint32_t index = 0; index < instanceCount_; ++index) {
		const Transform3D& transform = transforms_[index];
		instancingData[index].World = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		instancingData[index].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	// --- マテリアル（UV変換行列を組み立てて書き込む）---
	Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransform_.scale);
	uvTransformMatrix *= MakeRotateZMatrix(uvTransform_.rotate.z);
	uvTransformMatrix *= MakeTranslateMatrix(uvTransform_.translate);
	material_.uvTransform = uvTransformMatrix;
	materialCB_.Write(frameIndex, material_);
}

void InstancedObject3D::Draw() const {
	if (instanceCount_ == 0) {
		return;
	}

	RenderContext* context = RenderContext::GetInstance();
	ID3D12GraphicsCommandList* commandList = context->GetCommandList();
	PipelineManager* pipelineManager = PipelineManager::GetInstance();
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();

	// --- パーティクル用のRootSignatureと、通常のαブレンドのPSOへ切り替える ---
	// （RootSignatureを変えると設定済みのCBV等は無効になるため、使うものはすべて設定し直す）
	commandList->SetGraphicsRootSignature(pipelineManager->GetParticleRootSignature());
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kInstanced);

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

	// --- 描画！インスタンスの数ぶんを1回の描画命令でまとめて描く ---
	mesh_->Draw(commandList, instanceCount_);

	// --- 後に続く3D描画のために、標準のRootSignature・CBV・PSOへ戻す ---
	context->BindStandardState(commandList);
}

void InstancedObject3D::SetInstanceCount(uint32_t instanceCount) {
	// 確保したResourceの数を超えて描くと、GPUが範囲外を読んでしまう
	assert(instanceCount <= GetMaxInstanceCount());
	instanceCount_ = instanceCount;
}

Transform3D& InstancedObject3D::GetTransform(uint32_t index) {
	assert(index < GetMaxInstanceCount());
	return transforms_[index];
}

const Transform3D& InstancedObject3D::GetTransform(uint32_t index) const {
	assert(index < GetMaxInstanceCount());
	return transforms_[index];
}

void InstancedObject3D::SetTexture(const std::string& textureFilePath) {
	SetTextureHandle(TextureManager::GetInstance()->Load(textureFilePath));
}

} // namespace Engine
