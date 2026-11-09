#pragma once
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
namespace Engine {

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

} // namespace Engine
