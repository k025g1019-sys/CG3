#pragma once
#include <string>
#include <vector>
#include "Engine/Rendering/VertexData.h"
namespace Engine {

struct MaterialData {
	std::string textureFilePath;
};
struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

} // namespace Engine
