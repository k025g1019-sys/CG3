#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "Engine/Rendering/VertexData.h"
namespace Engine {

/// <summary>
/// mtlファイルの1マテリアル分の情報（newmtl単位）。
/// </summary>
struct MaterialData {
	std::string textureFilePath;  // map_Kdのテクスチャパス（無いマテリアルでは空）
};

/// <summary>
/// OBJのo/g/usemtl単位の描画範囲。頂点はModelData::verticesに連結して持ち、
/// ここでは開始位置と個数だけを指す。
/// </summary>
struct SubMeshData {
	std::string name;          // o/gの名前（無ければ空）
	std::string materialName;  // usemtlのマテリアル名（無ければ空）
	uint32_t vertexStart = 0;  // vertices内の開始位置
	uint32_t vertexCount = 0;  // 頂点数
};

struct ModelData {
	std::vector<VertexData> vertices;                         // 全サブメッシュ連結の頂点列
	std::vector<SubMeshData> subMeshes;                       // 描画範囲（面を持つOBJなら最低1個）
	std::unordered_map<std::string, MaterialData> materials;  // newmtl名 → マテリアル
};

/// <summary>
/// OBJファイルを読み込む。o/g/usemtlでサブメッシュに分割し、mtllibのマテリアルも取り込む。
/// 面は三角形限定。頂点は「位置のみ」「位置/UV」「位置//法線」「位置/UV/法線」の全形式に対応し、
/// UVが無い場合は(0,0)、法線が無い場合は(0,0,-1)を補う。
/// </summary>
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

/// <summary>
/// mtlファイルを読み込み、マテリアル名（newmtl）ごとの情報を返す。
/// </summary>
std::unordered_map<std::string, MaterialData> LoadMaterialTemplateFile(
	const std::string& directoryPath, const std::string& filename);

} // namespace Engine
