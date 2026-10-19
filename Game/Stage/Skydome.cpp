#include "Game/Stage/Skydome.h"

using namespace Engine;

#pragma region 初期化

void Skydome::Initialize() {
	// 天球のモデル（空の画像は mtl の map_Kd から読み込まれる）。陰影なしにして、画像の色をそのまま出す
	object_.Initialize("resources/skydome/skydome.obj", LightingMode::kNone);

	// ステージの中心（原点）を囲むように置く
	Transform3D& transform = object_.GetTransform();
	transform.scale = { kRadius, kRadius, kRadius };
	transform.translate = { 0.0f, 0.0f, 0.0f };
}

#pragma endregion

#pragma region 更新

void Skydome::Update() {
	object_.Update();
}

#pragma endregion

#pragma region 描画

void Skydome::Draw() const {
	object_.Draw();
}

#pragma endregion
