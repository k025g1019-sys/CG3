#include "Object/ScreenText.h"

#include <cmath>

#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/RenderContext.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

void ScreenText::Initialize(const std::string& bodyObjFilePath, const std::string& faceObjFilePath, int lineIndex) {
	// 文字が背景や光源の向きによらず読みやすいよう、どちらもライティングなしで描く
	body_.Initialize(bodyObjFilePath, LightingMode::kNone);
	face_.Initialize(faceObjFilePath, LightingMode::kNone);
	body_.SetColor(sideColor_);
	face_.SetColor(color_);
	lineIndex_ = lineIndex;
}

void ScreenText::Update(const Matrix4x4& viewMatrix, float fovY, float aspectRatio) {
	// --- カメラから distance_ 離れた面で、画面に映る範囲の半分の高さ・幅 ---
	const float halfHeight = distance_ * std::tan(fovY * 0.5f);
	const float halfWidth = halfHeight * aspectRatio;

	// 文字の大きさと余白（どちらも画面の高さ = halfHeight * 2 に対する割合で決める）
	const float scale = halfHeight * 2.0f * heightRatio_;
	const float margin = halfHeight * 2.0f * marginRatio_;

	// --- カメラから見た位置（カメラ空間。+Xが右、+Yが上、+Zが奥）---
	// OBJの原点が文字の左上なので、画面の左上の角から余白ぶん内側に原点を置く
	// （2行目以降は、行の高さ × 行番号ぶん下へずらす）
	// 文字は回転させず、画面と平行に置く（傾けると遠い側が小さく見え、行が斜めになるため）
	const float lineOffset = scale * kLineSpacing * float(lineIndex_);
	Transform3D transform{};
	transform.scale = { scale, scale, scale };
	transform.rotate = { 0.0f, 0.0f, 0.0f };
	transform.translate = { -halfWidth + margin, halfHeight - margin - lineOffset, distance_ };
	body_.GetTransform() = transform;
	face_.GetTransform() = transform;

	// --- 本体（厚み）だけを斜めにずらす（シアー）---
	// カメラ空間で、表面の面（z = frontZ）より奥にある点ほど、x・yを (z - frontZ) × 傾き だけずらす。
	//   x' = x + (z - frontZ) * shearX,  y' = y + (z - frontZ) * shearY
	// 表面の面上の点は動かないので、本体の表は表面とぴったり重なり、奥の面だけがずれて側面がのぞく。
	// 傾きは「カメラから文字への視線の傾き」＋「厚みを見せたい向き（depthDirection_）」にする。
	// 前者で、画面のどこに置いても視線と側面が平行になる（＝何もしなければ側面は見えない）ので、
	// 側面の見え方は後者だけで決まり、画面の大きさや位置によらず同じになる。
	const Vector3 localCenter = body_.GetMesh()->GetLocalCenter();
	const Vector3 center = {
		transform.translate.x + localCenter.x * scale,
		transform.translate.y + localCenter.y * scale,
		transform.translate.z + localCenter.z * scale };
	const float frontZ = transform.translate.z + face_.GetMesh()->GetLocalCenter().z * scale;
	const float shearX = center.x / center.z + depthDirection_.x;
	const float shearY = center.y / center.z + depthDirection_.y;

	Matrix4x4 shearMatrix = MakeIdentity4x4();
	shearMatrix.m[2][0] = shearX;
	shearMatrix.m[2][1] = shearY;
	shearMatrix.m[3][0] = -frontZ * shearX;
	shearMatrix.m[3][1] = -frontZ * shearY;

	// --- カメラのワールド行列（ビュー行列の逆行列）を親にして、カメラに付いて動かす ---
	const Matrix4x4 cameraWorldMatrix = Inverse(viewMatrix);
	body_.SetColor(sideColor_);
	face_.SetColor(color_);
	body_.Update(shearMatrix * cameraWorldMatrix);
	face_.Update(cameraWorldMatrix);
}

void ScreenText::Draw() const {
	ID3D12GraphicsCommandList* commandList = RenderContext::GetInstance()->GetCommandList();
	PipelineManager* pipelineManager = PipelineManager::GetInstance();

	// 深度テストをしないPSOに切り替えて描く（先に描いたどの物体よりも手前に表示される）。
	// RootSignatureは標準と同じなので、設定済みのカメラ・ライトのCBVはそのまま使える
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kOverlay);

	// 深度を使わないので、描いた順に上書きされる。
	// 厚みのある本体（側面の色）を先に描き、その上に表面を描いて、表面の文字の形を必ず見せる
	body_.Draw();
	face_.Draw();

	// 後に続く3D描画のために、標準のPSOへ戻す
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kStandard);
}

#ifdef USE_IMGUI
void ScreenText::DrawImGui(const char* label) {
	if (ImGui::TreeNode(label)) {
		ImGui::PushID(label);
		ImGui::DragFloat2("Depth Direction", &depthDirection_.x, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("Distance", &distance_, 0.05f, 0.2f, 50.0f);
		ImGui::DragFloat("Height (screen ratio)", &heightRatio_, 0.001f, 0.005f, 0.5f);
		ImGui::DragFloat("Margin (screen ratio)", &marginRatio_, 0.001f, 0.0f, 0.5f);
		ImGui::ColorEdit4("Color", &color_.x);
		ImGui::ColorEdit4("Side Color", &sideColor_.x);
		ImGui::PopID();
		ImGui::TreePop();
	}
}
#endif
