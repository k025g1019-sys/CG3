#include "Engine/Rendering/MeshManager.h"

#include <cassert>
#include <vector>

#include "Engine/Core/DirectXCore.h"
#include "Engine/Geometry/GeometryGenerator.h"

namespace Engine {

MeshManager* MeshManager::GetInstance() {
	static MeshManager instance;
	return &instance;
}

Mesh* MeshManager::Load(const std::string& objFilePath) {
	// 読み込み済みならそれを返す
	auto it = meshes_.find(objFilePath);
	if (it != meshes_.end()) {
		return it->second.get();
	}

	// "resources/player.obj" → ディレクトリ "resources" とファイル名 "player.obj" に分ける
	const size_t separator = objFilePath.find_last_of("/\\");
	const std::string directory =
		(separator == std::string::npos) ? "." : objFilePath.substr(0, separator);
	const std::string filename =
		(separator == std::string::npos) ? objFilePath : objFilePath.substr(separator + 1);

	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
	mesh->CreateFromObj(DirectXCore::GetInstance()->GetDevice(), directory, filename);

	Mesh* result = mesh.get();
	meshes_.emplace(objFilePath, std::move(mesh));
	return result;
}

Mesh* MeshManager::GetPrimitive(Primitive primitive) {
	assert(primitive < Primitive::kCount);
	std::unique_ptr<Mesh>& slot = primitives_[size_t(primitive)];
	if (slot) {
		return slot.get();
	}

	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();
	slot = std::make_unique<Mesh>();

	switch (primitive) {
	case Primitive::kCube:
		slot->CreateCube(device);
		break;

	case Primitive::kSphere: {
		// 生成される球は半径1なので、半分に縮めて直径1（立方体と同じ大きさ）にそろえる
		std::vector<VertexData> vertices = GenerateSphereVertices(kSphereSubdivision);
		for (VertexData& vertex : vertices) {
			vertex.position.x *= 0.5f;
			vertex.position.y *= 0.5f;
			vertex.position.z *= 0.5f;
		}
		slot->Create(device, vertices.data(), uint32_t(vertices.size()));
		break;
	}

	case Primitive::kPlane: {
		std::vector<VertexData> vertices = GeneratePlaneVertices();
		std::vector<uint32_t> indices = GeneratePlaneIndices();
		slot->Create(device,
			vertices.data(), uint32_t(vertices.size()),
			indices.data(), uint32_t(indices.size()));
		break;
	}

	case Primitive::kCount:
		break;
	}

	return slot.get();
}

void MeshManager::Finalize() {
	meshes_.clear();
	for (std::unique_ptr<Mesh>& primitive : primitives_) {
		primitive.reset();
	}
}

} // namespace Engine
