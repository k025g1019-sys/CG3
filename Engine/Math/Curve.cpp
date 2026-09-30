#include "Engine/Math/Curve.h"

#include <cassert>

#include "Engine/Math/Matrix4x4.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace Engine {

Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t) {
	// 制御点を結ぶ2本の線分をそれぞれtで補間し、その2点をさらにtで補間する（ド・カステリョのアルゴリズム）
	const Vector3 p01 = Lerp(p0, p1, t);
	const Vector3 p12 = Lerp(p1, p2, t);
	return Lerp(p01, p12, t);
}

Vector3 Curve::GetPoint(float t) const {
	return Bezier(controlPoints_[0], controlPoints_[1], controlPoints_[2], t);
}

void Curve::SetControlPoint(int index, const Vector3& point) {
	assert(index >= 0 && index < 3);
	controlPoints_[size_t(index)] = point;
}

#ifdef USE_IMGUI
void Curve::DrawImGui(const char* label) {
	if (ImGui::TreeNode(label)) {
		ImGui::DragFloat3("controlPoints[0]", &controlPoints_[0].x, 0.01f);
		ImGui::DragFloat3("controlPoints[1]", &controlPoints_[1].x, 0.01f);
		ImGui::DragFloat3("controlPoints[2]", &controlPoints_[2].x, 0.01f);
		ImGui::TreePop();
	}
}
#endif

} // namespace Engine
