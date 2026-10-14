#include "Sandbox/Scene/GameScene.h"

#include "Engine/Rendering/DebugDraw.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void GameScene::OnInitializeObjects() {
	// --- 板ポリ（plane.obj）のパーティクルを、最大kNumMaxInstance個まで出せるようにする ---
	particles_.Initialize("resources/plane.obj", kNumMaxInstance);

	// 丸い粒のテクスチャに差し替える（既定はplane.mtl由来のuvChecker.png）
	particles_.SetTexture("resources/circle.png");

	// --- エミッタの設定（0.5秒ごとに3個ずつ発生させる）---
	emitter_.translate = { 0.0f, 0.0f, 0.0f };      // 発生の基準となる位置
	emitter_.range = { 1.0f, 1.0f, 1.0f };          // 発生範囲（位置から ±1 の箱の中）
	emitter_.count = 3;                             // 1回あたりの発生数
	emitter_.frequency = 0.5f;                      // 発生頻度（0.5秒ごとに発生）
	emitter_.frequencyTime = 0.0f;                  // 発生頻度計測用タイマー（0で初期化）
	emitter_.velocity = { 0.0f, 0.0f, 0.0f };       // パーティクルの速度の基準
	emitter_.velocityRange = { 1.0f, 1.0f, 1.0f };  // 速度の幅（-1～1のランダムな速度になる）
	emitter_.lifeTimeMin = 1.0f;                    // パーティクルの生存時間（1～3秒）
	emitter_.lifeTimeMax = 3.0f;
	emitter_.color = { 0.5f, 0.5f, 0.5f, 1.0f };    // 初期色の基準
	emitter_.colorRange = { 0.5f, 0.5f, 0.5f, 0.0f };  // 初期色の幅（RGBが0～1のランダムな色になる。0にすると全て基準の色）
}

void GameScene::OnUpdateObjects() {
	// エミッタの発生頻度計測用タイマーを進め、発生するタイミングになったらパーティクルを出す
	if (emitter_.Update()) {
		particles_.Emit(emitter_);
	}

	// 各パーティクルの移動・消滅をし、ワールド行列と色をStructuredBufferへ書き込む
	// （ビルボード用に、描画に使うカメラ（デバッグカメラが有効ならそちら）のビュー行列を渡す）
	particles_.Update(CalcViewMatrix());

	// エミッタの範囲表示（発生範囲の箱を線で描く）
	if (showEmitterRange_) {
		DebugDraw::DrawAABB(emitter_.CalcRangeAABB(), { 1.0f, 1.0f, 0.2f, 1.0f });
	}
}

void GameScene::OnDrawObjects() {
	// 出ているパーティクルを1回の描画命令（DrawInstanced）で描く
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

		// 今出ているパーティクルの数 / 最大数
		ImGui::Text("Particle Count: %u / %u", particles_.GetParticleCount(), particles_.GetMaxParticleCount());

		// 1回ぶん（エミッタのEmit Countの数）をその場で発生させる / 出ているパーティクルをすべて消す
		if (ImGui::Button("Emit")) {
			particles_.Emit(emitter_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear")) {
			particles_.Clear();
		}

		// ビルボード（billboardMatrix）を利用する・しないの切り替え
		bool useBillboard = particles_.IsBillboardEnabled();
		if (ImGui::Checkbox("Use Billboard", &useBillboard)) {
			particles_.SetBillboardEnabled(useBillboard);
		}

		// 全パーティクル共通の色（パーティクルごとの色に掛け合わされる）
		ImGui::ColorEdit4("Color", &particles_.GetMaterial().color.x);

		// --- エミッタ ---
		ImGui::SeparatorText("Emitter");

		// 1回あたりの発生数（＋/－ボタンで増減できる。0～最大数）
		int emitCount = static_cast<int>(emitter_.count);
		if (ImGui::InputInt("Emit Count", &emitCount, 1, 10)) {
			const int maxCount = static_cast<int>(particles_.GetMaxParticleCount());
			if (emitCount < 0) {
				emitCount = 0;
			}
			if (emitCount > maxCount) {
				emitCount = maxCount;
			}
			emitter_.count = static_cast<uint32_t>(emitCount);
		}

		// 発生の基準となる位置と、発生範囲（位置から ± の幅）
		ImGui::DragFloat3("Emitter Translate", &emitter_.translate.x, 0.01f, -100.0f, 100.0f);
		ImGui::DragFloat3("Emitter Range", &emitter_.range.x, 0.01f, 0.0f, 100.0f);

		// エミッタの範囲表示の切り替え
		ImGui::Checkbox("Show Emitter Range", &showEmitterRange_);

		// 発生頻度（秒）と、発生頻度計測用タイマーの現在値
		ImGui::DragFloat("Frequency (sec)", &emitter_.frequency, 0.01f, 0.01f, 10.0f);
		ImGui::Text("Frequency Time: %.2f", emitter_.frequencyTime);

		// パーティクルの速度（基準 ± 幅）
		ImGui::DragFloat3("Velocity", &emitter_.velocity.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat3("Velocity Range", &emitter_.velocityRange.x, 0.01f, 0.0f, 10.0f);

		// パーティクルの生存時間（最小～最大）
		ImGui::DragFloatRange2("Life Time (sec)", &emitter_.lifeTimeMin, &emitter_.lifeTimeMax, 0.01f, 0.1f, 10.0f);

		// 初期色（基準色 ± 幅。幅が0なら全て基準色になる）
		ImGui::ColorEdit4("Base Color", &emitter_.color.x);
		ImGui::DragFloat4("Color Range", &emitter_.colorRange.x, 0.01f, 0.0f, 1.0f);

		ImGui::PopID();
		ImGui::TreePop();
	}
}

void GameScene::OnDrawCullingImGui() {
	// Instancingでまとめて描くため、インスタンスごとの視錐台カリングはしない
	ImGui::Text("Particles (instancing): not culled");
}
#endif
