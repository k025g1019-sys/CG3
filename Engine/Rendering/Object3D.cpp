#include "Engine/Rendering/Object3D.h"

#include <cassert>
#include <cmath>

#include "Engine/Core/DirectXCore.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Rendering/Mesh.h"

namespace Engine {

void Object3D::Initialize(
	ID3D12Device* device, Mesh* mesh, uint32_t textureHandle, LightingMode lightingMode) {
	assert(mesh != nullptr);

	mesh_ = mesh;
	textureHandle_ = textureHandle;

	// サブメッシュ（OBJのo/g/usemtl単位）ごとにmtl由来のテクスチャを解決する。
	// map_Kdを持たないマテリアル（suzanne等）は白テクスチャで代用し、
	// material_.color × ライティングの単色で描けるようにする
	subMeshTextureHandles_.clear();
	textureOverridden_ = false;
	TextureManager* textureManager = TextureManager::GetInstance();
	for (uint32_t i = 0; i < mesh_->GetSubMeshCount(); ++i) {
		const Mesh::SubMesh& subMesh = mesh_->GetSubMesh(i);
		subMeshTextureHandles_.push_back(
			subMesh.textureFilePath.empty()
				? textureManager->GetWhiteTexture()
				: textureManager->Load(subMesh.textureFilePath));
	}

	// マテリアルはサブメッシュごとに1個（サブメッシュを持たないメッシュは1個だけ）
	const uint32_t materialCount =
		mesh_->GetSubMeshCount() > 0 ? mesh_->GetSubMeshCount() : 1u;
	materials_.assign(materialCount, Material{});
	uvTransforms_.assign(
		materialCount,
		Transform3D{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } });
	for (Material& material : materials_) {
		material.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		material.lightingMode = lightingMode;
		material.uvTransform = MakeIdentity4x4();
	}

	transformCB_.Create(device, DirectXCore::kFramesInFlight);
	materialCBs_.clear();
	materialCBs_.resize(materialCount);
	for (ConstantBuffer<Material>& materialCB : materialCBs_) {
		materialCB.Create(device, DirectXCore::kFramesInFlight);
	}
}

void Object3D::Update(const Frustum3D& frustum) {
	// 視錐台カリング判定（Outsideの場合はDrawで描画をスキップする）
	visibility_ = ClassifyFrustum(frustum, CalcWorldBoundingSphere());

	// ワールド行列を計算して定数バッファへ書き込む
	Matrix4x4 world = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	TransformationMatrix transformData{ world };

	uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	transformCB_.Write(frameIndex, transformData);

	// マテリアルごとにUV変換行列を組み立てて定数バッファへ書き込む
	for (uint32_t i = 0; i < uint32_t(materials_.size()); ++i) {
		Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransforms_[i].scale);
		uvTransformMatrix *= MakeRotateZMatrix(uvTransforms_[i].rotate.z);
		uvTransformMatrix *= MakeTranslateMatrix(uvTransforms_[i].translate);
		materials_[i].uvTransform = uvTransformMatrix;

		materialCBs_[i].Write(frameIndex, materials_[i]);
	}
}

void Object3D::Draw(ID3D12GraphicsCommandList* commandList) const {
	if (!IsVisible(visibility_)) {
		return;
	}

	uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	commandList->SetGraphicsRootConstantBufferView(1, transformCB_.GetGPUAddress(frameIndex));

	TextureManager* textureManager = TextureManager::GetInstance();
	if (subMeshTextureHandles_.empty()) {
		// サブメッシュを持たないメッシュ（三角形・球など）は1回で描く
		commandList->SetGraphicsRootConstantBufferView(0, materialCBs_[0].GetGPUAddress(frameIndex));
		commandList->SetGraphicsRootDescriptorTable(3, textureManager->GetSrvHandleGPU(textureHandle_));
		mesh_->Draw(commandList);
	} else {
		// サブメッシュごとにマテリアルCBVとテクスチャを切り替えて描く
		// （テクスチャ一括上書き中もマテリアルはサブメッシュごとの値を使う）
		for (uint32_t i = 0; i < uint32_t(subMeshTextureHandles_.size()); ++i) {
			commandList->SetGraphicsRootConstantBufferView(
				0, materialCBs_[i].GetGPUAddress(frameIndex));
			commandList->SetGraphicsRootDescriptorTable(
				3, textureManager->GetSrvHandleGPU(
					textureOverridden_ ? textureHandle_ : subMeshTextureHandles_[i]));
			mesh_->DrawSubMesh(commandList, i);
		}
	}
}

Sphere Object3D::CalcWorldBoundingSphere() const {
	Matrix4x4 world = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Vector3 center = Transform(mesh_->GetLocalCenter(), world);

	// 拡大率の最大成分で半径をスケールする
	// （std::maxはWindows.hのmin/maxマクロと衝突するため比較で求める）
	float maxScale = std::fabs(transform_.scale.x);
	if (std::fabs(transform_.scale.y) > maxScale) { maxScale = std::fabs(transform_.scale.y); }
	if (std::fabs(transform_.scale.z) > maxScale) { maxScale = std::fabs(transform_.scale.z); }

	return Sphere{ center, mesh_->GetLocalRadius() * maxScale };
}

} // namespace Engine
