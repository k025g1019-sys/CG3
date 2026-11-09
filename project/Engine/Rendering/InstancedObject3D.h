#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../Math/Matrix4x4.h"
#include "../Math/Vector4.h"
#include "ConstantBuffer.h"
#include "Material.h"
#include "StructuredBuffer.h"
#include "TransformData3D.h"

namespace Engine {

class Mesh;

/// <summary>
/// 同じメッシュ（板ポリなど）を、インスタンスごとに別のTransformで、
/// 1回の描画命令（DrawInstanced）でまとめて描くオブジェクト。
/// ParticleSystemと違い、インスタンスは発生・消滅せず、数と位置は使う側が決める。
///   Initialize : メッシュと、描画できるインスタンスの最大数を決める（最初は最大数ぶん描く）
///   Update     : 各インスタンスのワールド行列をStructuredBufferへ書き込む（毎フレーム、Drawより前に呼ぶ）
///   Draw       : パーティクル用のRootSignatureと、通常のαブレンド・深度書き込みありのPSOで描画する
///                （シーンのOnDrawの中で呼ぶ）
/// インスタンスごとのワールド行列と色はStructuredBuffer（VSのt0）、ビュー射影は視点ごとの共有CBuffer（VSのb1）で渡す
/// （シェーダーはパーティクルと共有する）。
/// マテリアル（色・UV変換）とテクスチャは全インスタンス共通で、ライティングはしない。
/// </summary>
class InstancedObject3D {
public:

    /// <summary>
    /// OBJファイルで初期化する（メッシュはMeshManagerが読み込み・共有する。テクスチャは最初のサブメッシュのmtl由来）
    /// </summary>
    /// <param name="objFilePath">実行ディレクトリからの相対パス（例: "resources/plane.obj"）</param>
    /// <param name="maxInstanceCount">描画できるインスタンスの最大数（この数ぶんのGPUリソースを確保する）</param>
    void Initialize(const std::string& objFilePath, uint32_t maxInstanceCount);

    // 各インスタンスのワールド行列とマテリアルを書き込む（毎フレーム、Drawより前に呼ぶ）
    void Update();

    // 描画する（シーンのOnDrawの中で呼ぶ）。描き終わったら標準のRootSignature・PSOへ戻す。
    // インスタンスごとの視錐台カリングはしない（全インスタンスをまとめて描く）
    void Draw() const;

    // 描画するインスタンス数（0～最大数。最大数を超えるとassertで止まる。少ない分には問題ない）
    void SetInstanceCount(uint32_t instanceCount);
    uint32_t GetInstanceCount() const { return instanceCount_; }

    // 描画できるインスタンスの最大数（Initializeで確保した数）
    uint32_t GetMaxInstanceCount() const { return uint32_t(transforms_.size()); }

    // インスタンスごとのTransform（indexは0～最大数-1）
    Transform3D& GetTransform(uint32_t index);
    const Transform3D& GetTransform(uint32_t index) const;

    // 全インスタンス共通のマテリアル（色など。lightingModeは使わない）
    Material& GetMaterial() { return material_; }

    // UV変換（scale/rotate.z/translateを編集し、UpdateでMaterial::uvTransformの行列に変換される）
    Transform3D& GetUVTransform() { return uvTransform_; }

    // テクスチャを設定する（既定はOBJのmtl由来。map_Kdが無ければ白）
    void SetTextureHandle(uint32_t textureHandle) { textureHandle_ = textureHandle; }
    uint32_t GetTextureHandle() const { return textureHandle_; }

    // テクスチャをファイルから読み込んで設定する（例: "resources/uvChecker.png"）
    void SetTexture(const std::string& textureFilePath);

    Mesh* GetMesh() const { return mesh_; }

private:

    // インスタンス1個ぶんのGPUへ送るデータ（Particle.VS.hlslのParticleForGPUと一致させる）
    struct InstanceForGPU {
        Matrix4x4 World;
        Vector4 color;
    };

private:

    Mesh* mesh_ = nullptr;  // 非所有（MeshManagerが所有）

    uint32_t textureHandle_ = 0;

    // 全インスタンス共通のマテリアル（UpdateでmaterialCB_へ書き込む）
    Material material_{};

    Transform3D uvTransform_{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    ConstantBuffer<Material> materialCB_;

    // インスタンスごとのTransform（最大数ぶん）
    std::vector<Transform3D> transforms_;

    // 描画するインスタンス数（最大数以下）
    uint32_t instanceCount_ = 0;

    // Instancing用：最大数ぶんのワールド行列と色を格納するStructuredBuffer（VSのt0）
    StructuredBuffer<InstanceForGPU> instancingBuffer_;
};

} // namespace Engine
