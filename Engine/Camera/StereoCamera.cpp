#include "Engine/Camera/StereoCamera.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Graphics/StereoRenderer.h"
#include "Engine/Math/Vector3.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace Engine {

void StereoCamera::Initialize(ID3D12Device* device) {
	viewProjectionCB_.Create(device, DirectXCore::kFramesInFlight * StereoRenderer::kMaxViewCount);
}

void StereoCamera::Update(const Matrix4x4& view, const Matrix4x4& projection) {
	// 中心カメラから各視点（眼）を作る。
	// カメラは平行配置（toe-in不使用）。収束面で視差ゼロになるよう射影にシアーを加える（オフアクシス射影）。
	Matrix4x4 centerWorld = Inverse(view);
	Vector3 cameraRight = Normalize({ centerWorld.m[0][0], centerWorld.m[0][1], centerWorld.m[0][2] });
	Vector3 cameraUp = Normalize({ centerWorld.m[1][0], centerWorld.m[1][1], centerWorld.m[1][2] });
	Vector3 cameraPos = { centerWorld.m[3][0], centerWorld.m[3][1], centerWorld.m[3][2] };
	float xScale = projection.m[0][0];
	float yScale = projection.m[1][1];

	// 視線追跡による頭位置オフセット（カメラ右方向hx・上方向hy）。
	// 視点オフセットeと同じ仕組み（平行移動＋射影シアー）に乗せるため、収束面では視差ゼロのまま、
	// 物体だけが運動視差で角度を変えて見える（窓越しに覗き込む効果）。立体視OFF（1視点）でも効く。
	float hx = 0.0f;
	float hy = 0.0f;
	if (eyeTrackingEnabled_) {
		hx = gazeX_ * gazeMoveScaleX_;
		hy = gazeY_ * gazeMoveScaleY_;
	}

	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();
	const uint32_t viewCount = StereoRenderer::GetInstance()->GetActiveViewCount();
	for (uint32_t v = 0; v < viewCount; ++v) {
		// 視点オフセットe：視点0=左(-)、最終視点=右(+)。2視点なら -sep/2, +sep/2。1視点（Off）なら中央。
		float e = 0.0f;
		if (viewCount >= 2) {
			float t = float(v) / float(viewCount - 1);  // 0..1
			e = (t - 0.5f) * eyeSeparation_;
		}
		// 水平オフセットは「眼の分離e」と「頭の左右移動hx」の合算。
		float ox = e + hx;
		// 眼の位置は中心から右方向へox、上方向へhy。向きは中心と同じ（平行配置）。
		Matrix4x4 eyeWorld = centerWorld;
		eyeWorld.m[3][0] = cameraPos.x + cameraRight.x * ox + cameraUp.x * hy;
		eyeWorld.m[3][1] = cameraPos.y + cameraRight.y * ox + cameraUp.y * hy;
		eyeWorld.m[3][2] = cameraPos.z + cameraRight.z * ox + cameraUp.z * hy;
		Matrix4x4 eyeView = Inverse(eyeWorld);

		// オフアクシス射影：水平シアーm[2][0]・垂直シアーm[2][1]を設定し、収束面で像を一致させる。
		Matrix4x4 eyeProjection = projection;
		eyeProjection.m[2][0] = xScale * ox / convergence_;
		eyeProjection.m[2][1] = yScale * hy / convergence_;

		viewProjectionCB_.Write(
			frameIndex * StereoRenderer::kMaxViewCount + v, eyeView * eyeProjection);
	}
}

D3D12_GPU_VIRTUAL_ADDRESS StereoCamera::GetViewProjectionAddress(
	uint32_t frameIndex, uint32_t viewIndex) const {
	return viewProjectionCB_.GetGPUAddress(frameIndex * StereoRenderer::kMaxViewCount + viewIndex);
}

#ifdef USE_IMGUI
void StereoCamera::DrawImGuiSection() {
	// 立体視の共有パラメータ（視点描画に効く）
	ImGui::Text("Stereoscopic (view)");
	ImGui::DragFloat("Eye Separation", &eyeSeparation_, 0.005f, 0.0f, 5.0f);
	ImGui::DragFloat("Convergence", &convergence_, 0.1f, 0.1f, 100.0f);

	ImGui::Separator();

	// 視線追跡による頭連動オフアクシスの強さ（視線ON/OFFは"Eye Tracking & Camera"のチェックボックス）
	ImGui::Text("Eye Tracking (head-coupled off-axis)");
	ImGui::Text("Status: %s", eyeTrackingEnabled_ ? "ON (tracking)" : "OFF");
	ImGui::DragFloat("Gaze Move X", &gazeMoveScaleX_, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Gaze Move Y", &gazeMoveScaleY_, 0.01f, 0.0f, 10.0f);
	ImGui::Text("Gaze: (%.2f, %.2f)", gazeX_, gazeY_);
}
#endif

} // namespace Engine
