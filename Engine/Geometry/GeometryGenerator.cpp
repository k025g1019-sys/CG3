#include "Engine/Geometry/GeometryGenerator.h"

#include <cmath>

#include "Engine/Math/Vector3.h"

namespace Engine {

std::vector<VertexData> GenerateSphereVertices(uint32_t subdivision) {
	const float pi = 3.1415926535f;

	std::vector<VertexData> vertices;
	vertices.resize(size_t(subdivision) * subdivision * 6);

	const float kLonEvery = 2.0f * pi / subdivision;
	const float kLatEvery = pi / subdivision;

	for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
		float lat0 = -pi / 2.0f + kLatEvery * latIndex;
		float lat1 = lat0 + kLatEvery;

		for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
			float lon0 = lonIndex * kLonEvery;
			float lon1 = (lonIndex + 1) * kLonEvery;

			uint32_t start = (latIndex * subdivision + lonIndex) * 6;

			Vector3 v0 = { cos(lat0) * cos(lon0), sin(lat0), cos(lat0) * sin(lon0) };
			Vector3 v1 = { cos(lat1) * cos(lon0), sin(lat1), cos(lat1) * sin(lon0) };
			Vector3 v2 = { cos(lat0) * cos(lon1), sin(lat0), cos(lat0) * sin(lon1) };
			Vector3 v3 = { cos(lat1) * cos(lon1), sin(lat1), cos(lat1) * sin(lon1) };

			vertices[start + 0].position = { v0.x, v0.y, v0.z, 1.0f };
			vertices[start + 1].position = { v1.x, v1.y, v1.z, 1.0f };
			vertices[start + 2].position = { v2.x, v2.y, v2.z, 1.0f };

			vertices[start + 3].position = { v2.x, v2.y, v2.z, 1.0f };
			vertices[start + 4].position = { v1.x, v1.y, v1.z, 1.0f };
			vertices[start + 5].position = { v3.x, v3.y, v3.z, 1.0f };

			// 半径1の球なので、頂点位置がそのまま法線になる
			vertices[start + 0].normal = v0;
			vertices[start + 1].normal = v1;
			vertices[start + 2].normal = v2;

			vertices[start + 3].normal = v2;
			vertices[start + 4].normal = v1;
			vertices[start + 5].normal = v3;

			float u0 = float(lonIndex) / subdivision;
			float u1 = float(lonIndex + 1) / subdivision;
			float v0_uv = 1.0f - float(latIndex) / subdivision;
			float v1_uv = 1.0f - float(latIndex + 1) / subdivision;

			vertices[start + 0].texcoord = { u0, v0_uv };
			vertices[start + 1].texcoord = { u0, v1_uv };
			vertices[start + 2].texcoord = { u1, v0_uv };

			vertices[start + 3].texcoord = { u1, v0_uv };
			vertices[start + 4].texcoord = { u0, v1_uv };
			vertices[start + 5].texcoord = { u1, v1_uv };
		}
	}

	return vertices;
}

std::vector<VertexData> GenerateCubeVertices() {
	// 各面を「外から見たときの右方向・上方向」で定義する。
	// 表面（時計回り）の巻き順になるよう、法線 = 上方向×右方向 を満たす組み合わせにしている。
	struct Face {
		Vector3 normal;  // 外向き法線
		Vector3 right;   // 外から見て右方向
		Vector3 up;      // 外から見て上方向
	};
	const Face faces[6] = {
		{ { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },   // 前(-Z)
		{ { 0.0f, 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },   // 後(+Z)
		{ { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },  // 左(-X)
		{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },    // 右(+X)
		{ { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },    // 上(+Y)
		{ { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },  // 下(-Y)
	};

	std::vector<VertexData> vertices;
	vertices.resize(6 * 4);

	for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex) {
		const Face& face = faces[faceIndex];
		const Vector3 center = face.normal * 0.5f;

		// 4頂点：0=左下 / 1=左上 / 2=右下 / 3=右上（テクスチャ座標は左上原点）
		const Vector3 corners[4] = {
			center - face.right * 0.5f - face.up * 0.5f,
			center - face.right * 0.5f + face.up * 0.5f,
			center + face.right * 0.5f - face.up * 0.5f,
			center + face.right * 0.5f + face.up * 0.5f,
		};
		const Vector2 texcoords[4] = {
			{ 0.0f, 1.0f },
			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },
		};

		for (uint32_t i = 0; i < 4; ++i) {
			VertexData& vertex = vertices[size_t(faceIndex) * 4 + i];
			vertex.position = { corners[i].x, corners[i].y, corners[i].z, 1.0f };
			vertex.texcoord = texcoords[i];
			vertex.normal = face.normal;
		}
	}

	return vertices;
}

std::vector<uint32_t> GenerateCubeIndices() {
	std::vector<uint32_t> indices;
	indices.resize(6 * 6);

	for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex) {
		const uint32_t base = faceIndex * 4;
		uint32_t* face = &indices[size_t(faceIndex) * 6];
		// 左下→左上→右下 / 右下→左上→右上（時計回り＝表）
		face[0] = base + 0;
		face[1] = base + 1;
		face[2] = base + 2;
		face[3] = base + 2;
		face[4] = base + 1;
		face[5] = base + 3;
	}

	return indices;
}

std::vector<VertexData> GeneratePlaneVertices() {
	// 立方体の上面と同じ向き（外＝上(+Y)から見て右が+X、上が+Z）で、高さ0に置く
	const Vector3 normal = { 0.0f, 1.0f, 0.0f };
	const Vector3 right = { 1.0f, 0.0f, 0.0f };
	const Vector3 up = { 0.0f, 0.0f, 1.0f };

	// 4頂点：0=左下 / 1=左上 / 2=右下 / 3=右上（テクスチャ座標は左上原点）
	const Vector3 corners[4] = {
		-right * 0.5f - up * 0.5f,
		-right * 0.5f + up * 0.5f,
		right * 0.5f - up * 0.5f,
		right * 0.5f + up * 0.5f,
	};
	const Vector2 texcoords[4] = {
		{ 0.0f, 1.0f },
		{ 0.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
	};

	std::vector<VertexData> vertices(4);
	for (uint32_t i = 0; i < 4; ++i) {
		vertices[i].position = { corners[i].x, corners[i].y, corners[i].z, 1.0f };
		vertices[i].texcoord = texcoords[i];
		vertices[i].normal = normal;
	}

	return vertices;
}

std::vector<uint32_t> GeneratePlaneIndices() {
	// 左下→左上→右下 / 右下→左上→右上（上から見て時計回り＝表）
	return { 0, 1, 2, 2, 1, 3 };
}

} // namespace Engine
