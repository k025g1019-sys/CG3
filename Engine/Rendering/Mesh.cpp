#include "Engine/Rendering/Mesh.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>

#include "Engine/Geometry/GeometryGenerator.h"
#include "Engine/Geometry/LoadObjFile.h"
#include "Engine/Graphics/GpuResource.h"

namespace Engine {

void Mesh::Create(ID3D12Device* device, const VertexData* vertices, uint32_t vertexCount) {
	Create(device, vertices, vertexCount, nullptr, 0);
}

void Mesh::Create(
	ID3D12Device* device,
	const VertexData* vertices, uint32_t vertexCount,
	const uint32_t* indices, uint32_t indexCount) {

	// 再生成時（球の分割数変更等）に前のサブメッシュ範囲が残らないようにする
	subMeshes_.clear();

	// --- 頂点バッファ ---
	vertexCount_ = vertexCount;
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * vertexCount);
	vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vbv_.SizeInBytes = UINT(sizeof(VertexData) * vertexCount);
	vbv_.StrideInBytes = sizeof(VertexData);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices_));
	if (vertices != nullptr) {
		std::memcpy(mappedVertices_, vertices, sizeof(VertexData) * vertexCount);
	}

	// --- インデックスバッファ（任意）---
	indexCount_ = indexCount;
	if (indices != nullptr && indexCount > 0) {
		indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * indexCount);
		ibv_.BufferLocation = indexResource_->GetGPUVirtualAddress();
		ibv_.SizeInBytes = UINT(sizeof(uint32_t) * indexCount);
		ibv_.Format = DXGI_FORMAT_R32_UINT;
		uint32_t* mappedIndices = nullptr;
		indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndices));
		std::memcpy(mappedIndices, indices, sizeof(uint32_t) * indexCount);
	} else {
		indexResource_.Reset();
		ibv_ = {};
	}

	RecomputeBoundingSphere();
}

void Mesh::CreateFromObj(ID3D12Device* device, const std::string& directoryPath, const std::string& filename) {
	ModelData modelData = LoadObjFile(directoryPath, filename);
	Create(device, modelData.vertices.data(), uint32_t(modelData.vertices.size()));

	// サブメッシュ範囲を取り込み、マテリアル名からテクスチャパスを解決する。
	// ここでは文字列の解決のみ行い、テクスチャの読み込みはObject3D::Initializeに任せる
	// （mtlに存在しないパスが書かれていても、Object3Dで使わない限り事故にならない）
	subMeshes_.reserve(modelData.subMeshes.size());
	for (const SubMeshData& src : modelData.subMeshes) {
		SubMesh subMesh{ src.name, src.materialName, {}, src.vertexStart, src.vertexCount };
		auto it = modelData.materials.find(src.materialName);
		if (it != modelData.materials.end()) {
			subMesh.textureFilePath = it->second.textureFilePath;
		}
		subMeshes_.push_back(std::move(subMesh));
	}
}

void Mesh::CreateSphere(ID3D12Device* device, uint32_t subdivision) {
	std::vector<VertexData> vertices = GenerateSphereVertices(subdivision);
	Create(device, vertices.data(), uint32_t(vertices.size()));
}

void Mesh::CreateCube(ID3D12Device* device) {
	std::vector<VertexData> vertices = GenerateCubeVertices();
	std::vector<uint32_t> indices = GenerateCubeIndices();
	Create(device,
		vertices.data(), uint32_t(vertices.size()),
		indices.data(), uint32_t(indices.size()));
}

void Mesh::Draw(ID3D12GraphicsCommandList* commandList, uint32_t instanceCount) const {
	commandList->IASetVertexBuffers(0, 1, &vbv_);
	if (indexCount_ > 0) {
		commandList->IASetIndexBuffer(&ibv_);
		commandList->DrawIndexedInstanced(indexCount_, instanceCount, 0, 0, 0);
	} else {
		commandList->DrawInstanced(vertexCount_, instanceCount, 0, 0);
	}
}

void Mesh::DrawSubMesh(ID3D12GraphicsCommandList* commandList, uint32_t index) const {
	assert(index < subMeshes_.size());
	const SubMesh& subMesh = subMeshes_[index];
	commandList->IASetVertexBuffers(0, 1, &vbv_);
	commandList->DrawInstanced(subMesh.vertexCount, 1, subMesh.vertexStart, 0);
}

void Mesh::RecomputeBoundingSphere() {
	if (vertexCount_ == 0 || mappedVertices_ == nullptr) {
		localCenter_ = { 0.0f, 0.0f, 0.0f };
		localRadius_ = 0.0f;
		return;
	}

	// AABBの中心をバウンディング球の中心にする
	// （std::min/maxはWindows.hのmin/maxマクロと衝突するため、比較で求める）
	Vector3 minPos = { mappedVertices_[0].position.x, mappedVertices_[0].position.y, mappedVertices_[0].position.z };
	Vector3 maxPos = minPos;
	for (uint32_t i = 1; i < vertexCount_; ++i) {
		float px = mappedVertices_[i].position.x;
		float py = mappedVertices_[i].position.y;
		float pz = mappedVertices_[i].position.z;
		if (px < minPos.x) { minPos.x = px; }
		if (py < minPos.y) { minPos.y = py; }
		if (pz < minPos.z) { minPos.z = pz; }
		if (px > maxPos.x) { maxPos.x = px; }
		if (py > maxPos.y) { maxPos.y = py; }
		if (pz > maxPos.z) { maxPos.z = pz; }
	}
	localCenter_ = (minPos + maxPos) * 0.5f;

	// 中心から最も遠い頂点までの距離を半径にする
	float radiusSq = 0.0f;
	for (uint32_t i = 0; i < vertexCount_; ++i) {
		float dx = mappedVertices_[i].position.x - localCenter_.x;
		float dy = mappedVertices_[i].position.y - localCenter_.y;
		float dz = mappedVertices_[i].position.z - localCenter_.z;
		float distSq = dx * dx + dy * dy + dz * dz;
		if (distSq > radiusSq) {
			radiusSq = distSq;
		}
	}
	localRadius_ = std::sqrt(radiusSq);
}

} // namespace Engine
