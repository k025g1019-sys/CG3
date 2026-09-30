#pragma once

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <vector>

#include "Engine/Culling/FrustumCulling.h"
#include "Engine/Math/Matrix4x4.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/Primitive.h"
#include "Engine/Rendering/TransformData3D.h"
#include "Engine/Rendering/TransformationMatrix.h"

namespace Engine {

class Mesh;

/// <summary>
/// メッシュ＋Transform＋マテリアル＋テクスチャを組にした3D描画オブジェクト。
///   Initialize : 形状を決める（組み込みの形状 / OBJファイル / 自前のメッシュ）
///   Update     : 行列・マテリアルを定数バッファへ書き込む（毎フレーム、Drawより前に呼ぶ）
///   Draw       : 描画する（シーンのOnDrawの中で呼ぶ。視錐台の外なら描かない）
/// メッシュがサブメッシュ（OBJのo/g/usemtl単位）を持つ場合は、mtl由来のテクスチャと
/// マテリアル（色・ライティング・UVTransform）をサブメッシュごとに持ち、
/// 切り替えながら描画する。
/// </summary>
class Object3D {
public:

    // --- 初期化（どれか1つを呼ぶ）---

    /// <summary>
    /// 組み込みの形状で初期化する（仮モデル用。白一色なので SetColor で色を付ける）
    /// </summary>
    /// <param name="primitive">形状（立方体・球・平面）</param>
    /// <param name="lightingMode">ライティングの計算方式（なし/Lambert/Half Lambert）</param>
    void Initialize(Primitive primitive, LightingMode lightingMode = LightingMode::kHalfLambert);

    /// <summary>
    /// OBJファイルで初期化する（同じファイルは1回だけ読み込まれ、メッシュは共有される）
    /// </summary>
    /// <param name="objFilePath">実行ディレクトリからの相対パス（例: "resources/player.obj"）</param>
    /// <param name="lightingMode">ライティングの計算方式（なし/Lambert/Half Lambert）</param>
    void Initialize(const std::string& objFilePath, LightingMode lightingMode = LightingMode::kHalfLambert);

    /// <param name="mesh">形状（非所有。呼び出し側が生存期間を管理する）</param>
    /// <param name="textureHandle">TextureManagerのテクスチャハンドル
    /// （サブメッシュを持つメッシュではmtl由来のテクスチャが優先され、これは上書き用の初期値になる）</param>
    /// <param name="lightingMode">ライティングの計算方式（なし/Lambert/Half Lambert）</param>
    void Initialize(
        Mesh* mesh, uint32_t textureHandle,
        LightingMode lightingMode = LightingMode::kHalfLambert);

    // ワールド行列・マテリアルを定数バッファへ書き込む（毎フレーム、Drawより前に呼ぶ）。
    // ビュー射影は視点ごとの共有CBuffer（VSのb1）で供給されるため、ここでは扱わない。
    void Update();

    // 描画する（シーンのOnDrawの中で呼ぶ）。
    // シーンのカメラの視錐台の外にあるときは描画しない（視錐台カリング）。
    // （RootSignature・PSO・ライト・ビュー射影はシーン側で設定済みの前提）
    void Draw() const;

    // 現在のTransformとメッシュから、ワールド空間のバウンディング球を計算する（ピッキング・簡易当たり判定用）
    Sphere CalcWorldBoundingSphere() const;

    Transform3D& GetTransform() { return transform_; }
    const Transform3D& GetTransform() const { return transform_; }

    // マテリアル数（サブメッシュを持つメッシュはサブメッシュ数、それ以外は1）
    uint32_t GetMaterialCount() const { return uint32_t(materials_.size()); }

    // CPU側マテリアル（ImGuiで色・ライティングを編集し、Updateで定数バッファへ反映される）
    Material& GetMaterial(uint32_t index = 0) { return materials_[index]; }

    // 全マテリアルの色をまとめて設定する（仮モデルの色分けや、半透明にするときなど）
    void SetColor(const Vector4& color);

    // マテリアルごとのUV変換（ImGuiで編集し、UpdateでMaterial::uvTransform行列へ変換される）
    Transform3D& GetUVTransform(uint32_t index = 0) { return uvTransforms_[index]; }

    // テクスチャを設定する。サブメッシュを持つメッシュでは全サブメッシュの一括上書きになる
    void SetTextureHandle(uint32_t textureHandle) {
        textureHandle_ = textureHandle;
        textureOverridden_ = true;
    }
    uint32_t GetTextureHandle() const { return textureHandle_; }

    // テクスチャをファイルから読み込んで設定する（例: "resources/player.png"。SetTextureHandleと同じ扱い）
    void SetTexture(const std::string& textureFilePath);

    // テクスチャの一括上書きを解除し、サブメッシュごとのmtl由来テクスチャへ戻す
    void ClearTextureOverride() { textureOverridden_ = false; }
    bool IsTextureOverridden() const { return textureOverridden_; }

    // 直近のDrawでの視錐台カリングの結果
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

    // Updateで計算したワールド空間のバウンディング球（Drawの視錐台カリングに使う）
    Sphere worldBoundingSphere_{ { 0.0f, 0.0f, 0.0f }, 0.0f };

    // 直近のDrawで判定したカリング結果（Drawはconstだが判定結果だけは記録する）
    mutable FrustumVisibility visibility_ = FrustumVisibility::Inside;
};

} // namespace Engine
