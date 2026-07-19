#pragma once

#include <cstdint>
#include <d3d12.h>
#include <vector>

#include "Engine/Culling/FrustumCulling.h"
#include "Engine/Math/Matrix4x4.h"
#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/TransformData3D.h"
#include "Engine/Rendering/TransformationMatrix.h"

namespace Engine {

class Mesh;

/// <summary>
/// メッシュ＋Transform＋マテリアル＋テクスチャを組にした3D描画オブジェクト。
/// Update: 行列計算・定数バッファ書き込み・視錐台カリング判定
/// Draw:   カリング結果がOutsideでなければ描画コマンドを積む
/// メッシュがサブメッシュ（OBJのo/g/usemtl単位）を持つ場合は、mtl由来のテクスチャと
/// マテリアル（色・ライティング・UVTransform）をサブメッシュごとに持ち、
/// 切り替えながら描画する。
/// </summary>
class Object3D {
public:

    /// <param name="mesh">形状（非所有。呼び出し側が生存期間を管理する）</param>
    /// <param name="textureHandle">TextureManagerのテクスチャハンドル
    /// （サブメッシュを持つメッシュではmtl由来のテクスチャが優先され、これは上書き用の初期値になる）</param>
    /// <param name="lightingMode">ライティングの計算方式（なし/Lambert/Half Lambert）</param>
    void Initialize(
        ID3D12Device* device, Mesh* mesh, uint32_t textureHandle,
        LightingMode lightingMode = LightingMode::kHalfLambert);

    // ワールド行列・マテリアルの定数バッファ更新と視錐台カリング判定（毎フレーム呼ぶ）。
    // ビュー射影は視点ごとの共有CBuffer（VSのb1）で供給されるため、ここでは扱わない。
    void Update(const Frustum3D& frustum);

    // カリング結果がOutsideでなければ描画する
    // （RootSignature・PSO・トポロジ・ライトCBV・ビュー射影CBVは呼び出し側で設定済みの前提）
    void Draw(ID3D12GraphicsCommandList* commandList) const;

    // 現在のTransformとメッシュから、ワールド空間のバウンディング球を計算する（ピッキング用）
    Sphere CalcWorldBoundingSphere() const;

    Transform3D& GetTransform() { return transform_; }
    const Transform3D& GetTransform() const { return transform_; }

    // マテリアル数（サブメッシュを持つメッシュはサブメッシュ数、それ以外は1）
    uint32_t GetMaterialCount() const { return uint32_t(materials_.size()); }

    // CPU側マテリアル（ImGuiで色・ライティングを編集し、Updateで定数バッファへ反映される）
    Material& GetMaterial(uint32_t index = 0) { return materials_[index]; }

    // マテリアルごとのUV変換（ImGuiで編集し、UpdateでMaterial::uvTransform行列へ変換される）
    Transform3D& GetUVTransform(uint32_t index = 0) { return uvTransforms_[index]; }

    // テクスチャを設定する。サブメッシュを持つメッシュでは全サブメッシュの一括上書きになる
    void SetTextureHandle(uint32_t textureHandle) {
        textureHandle_ = textureHandle;
        textureOverridden_ = true;
    }
    uint32_t GetTextureHandle() const { return textureHandle_; }

    // テクスチャの一括上書きを解除し、サブメッシュごとのmtl由来テクスチャへ戻す
    void ClearTextureOverride() { textureOverridden_ = false; }
    bool IsTextureOverridden() const { return textureOverridden_; }

    FrustumVisibility GetVisibility() const { return visibility_; }

    Mesh* GetMesh() const { return mesh_; }

private:

    Mesh* mesh_ = nullptr;  // 非所有

    Transform3D transform_{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    // CPU側マテリアル（サブメッシュごとに1個。サブメッシュなしのメッシュは1個だけ）。
    // Updateで対応するmaterialCBs_へ書き込む
    std::vector<Material> materials_;

    // マテリアルごとのUV変換（scale/rotate.z/translateをImGuiで編集し、
    // UpdateでMaterial::uvTransformの行列に変換する）
    std::vector<Transform3D> uvTransforms_;

    uint32_t textureHandle_ = 0;

    // サブメッシュごとの解決済みテクスチャハンドル（サブメッシュを持たないメッシュでは空）
    std::vector<uint32_t> subMeshTextureHandles_;

    // trueの間はtextureHandle_で全サブメッシュを描く（SetTextureHandleによる一括上書き）
    bool textureOverridden_ = false;

    ConstantBuffer<TransformationMatrix> transformCB_;

    // マテリアルごとの定数バッファ（materials_と同じ並び）
    std::vector<ConstantBuffer<Material>> materialCBs_;

    // Updateで判定したカリング結果（Drawで参照する）
    FrustumVisibility visibility_ = FrustumVisibility::Inside;
};

} // namespace Engine
