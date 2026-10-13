#include "Sandbox/Scene/GameScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void GameScene::OnInitializeObjects() {
	// --- 板ポリ（plane.obj）のパーティクルをkNumInstance個発生させる ---
	// （位置・速度・色・生存時間はParticleSystemが乱数で決める）
	particles_.Initialize("resources/plane.obj", kNumInstance);

	// 丸い粒のテクスチャに差し替える（既定はplane.mtl由来のuvChecker.png）
	particles_.SetTexture("resources/circle.png");
}

void GameScene::OnUpdateObjects() {
	// 各パーティクルの移動・消滅と再発生をし、ワールド行列と色をStructuredBufferへ書き込む
	// （ビルボード用に、描画に使うカメラ（デバッグカメラが有効ならそちら）のビュー行列を渡す）
	particles_.Update(CalcViewMatrix());
}

void GameScene::OnDrawObjects() {
	// パーティクルkNumInstance個を1回の描画命令（DrawInstanced）で描く
	particles_.Draw();
}

#ifndef NDEBUG
void GameScene::AppendPickTargets(std::vector<DebugCamera::PickTarget>& /*targets*/) const {
	// パーティクルはObject3Dではないため、ピッキングの対象にしない
}
#endif

void GameScene::AppendPadTargets(std::vector<PadObjectController::Target>& /*targets*/) {
	// パーティクルはObject3Dではないため、パッド操作の対象にしない
}

#ifdef USE_IMGUI
void GameScene::OnDrawObjectsImGui() {
	if (ImGui::TreeNode("Particles (Instancing)")) {
		ImGui::PushID("Particles");

		// パーティクルの数（消滅したぶんはすぐ再発生するため、常にこの数が出ている）
		ImGui::Text("Particle Count: %u", particles_.GetParticleCount());

		// ビルボード（billboardMatrix）を利用する・しないの切り替え
		bool useBillboard = particles_.IsBillboardEnabled();
		if (ImGui::Checkbox("Use Billboard", &useBillboard)) {
			particles_.SetBillboardEnabled(useBillboard);
		}

		// 全パーティクル共通の色（パーティクルごとの色に掛け合わされる）
		ImGui::ColorEdit4("Color", &particles_.GetMaterial().color.x);

		ImGui::PopID();
		ImGui::TreePop();
	}
}

void GameScene::OnDrawCullingImGui() {
	// Instancingでまとめて描くため、インスタンスごとの視錐台カリングはしない
	ImGui::Text("Particles (instancing): not culled");
}
#endif
