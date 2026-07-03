#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Rendering/VertexData.h"

// 緯度経度分割の球（半径1・原点中心）の頂点列を生成する
std::vector<VertexData> GenerateSphereVertices(uint32_t subdivision);

// 1辺1・原点中心の立方体の頂点列を生成する
// （面ごとに法線を持つ4頂点×6面＝24頂点。各面にテクスチャ全体（UV 0..1）を貼る）
std::vector<VertexData> GenerateCubeVertices();

// GenerateCubeVerticesの頂点列に対応するインデックス列を生成する（6面×2三角形×3頂点＝36個）
std::vector<uint32_t> GenerateCubeIndices();
