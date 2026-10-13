#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "Engine/Math/Matrix4x4.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/StructuredBuffer.h"
#include "Engine/Rendering/TransformData3D.h"

namespace Engine {

class Mesh;

/// <summary>
/// 同じメッシュ（板ポリなど）を、パーティクルごとに別のTransform・色で、
/// 1回の描画命令（DrawInstanced）でまとめて描くパーティクル。
///   Initialize : メッシュと、パーティクルの数を決め、全パーティクルを乱数で発生させる
///   Update     : 移動・生存時間の更新をし、ワールド行列と色をStructuredBufferへ書き込む（毎フレーム、Drawより前に呼ぶ）
///   Draw       : パーティクル用のRootSignature・PSO（加算合成）で描画する（シーンのOnDrawの中で呼ぶ）
/// パーティクルは発生時に位置・速度・色・生存時間（1～3秒）を乱数で決め、生存時間に近づくほど薄くなる。
/// 生存時間を過ぎたものは消滅し、同じ数を保つようにすぐ新しく発生させる。
/// パーティクルごとのワールド行列と色はStructuredBuffer（VSのt0）、ビュー射影は視点ごとの共有CBuffer（VSのb1）で渡す。
/// マテリアル（色・UV変換）とテクスチャは全パーティクル共通で、ライティングはしない。
/// </summary>
class ParticleSystem {
public:

    /// <summary>
    /// OBJファイルで初期化する（メッシュはMeshManagerが読み込み・共有する。テクスチャは最初のサブメッシュのmtl由来）
    /// </summary>
    /// <param name="objFilePath">実行ディレクトリからの相対パス（例: "resources/plane.obj"）</param>
    /// <param name="particleCount">パーティクルの数（この数ぶんのGPUリソースを確保し、常にこの数を描く）</param>
    void Initialize(const std::string& objFilePath, uint32_t particleCount);

    // 各パーティクルを動かし、ワールド行列・色とマテリアルを書き込む（毎フレーム、Drawより前に呼ぶ）
    void Update();

    // 描画する（シーンのOnDrawの中で呼ぶ）。描き終わったら標準のRootSignature・PSOへ戻す。
    // パーティクルごとの視錐台カリングはしない（全パーティクルをまとめて描く）
    void Draw() const;

    // パーティクルの数（Initializeで決めた数）
    uint32_t GetParticleCount() const { return uint32_t(particles_.size()); }

    // 全パーティクル共通のマテリアル（パーティクルごとの色に掛け合わされる。lightingModeはパーティクルでは使わない）
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

    // パーティクル1個ぶんのデータ（CPU側で持つ）
    struct Particle {
        Transform3D transform_{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
        Vector3 velocity_{};                        // 1秒あたりの移動量
        Vector4 color_{ 1.0f, 1.0f, 1.0f, 1.0f };
        float lifeTime_ = 0.0f;                     // 生存時間（秒）
        float currentTime_ = 0.0f;                  // 発生してからの経過時間（秒）

        // 生存時間を過ぎたか（過ぎたパーティクルは消滅させる）
        bool IsDead() const { return currentTime_ >= lifeTime_; }
    };

    // パーティクル1個ぶんのGPUへ送るデータ（Particle.VS.hlslのParticleForGPUと一致させる）
    struct ParticleForGPU {
        Matrix4x4 World;
        Vector4 color;
    };

    // パーティクルを1個生成する（位置・速度は[-1,1]、色はRGBそれぞれ[0,1]、生存時間は1～3秒の乱数で決める）
    static Particle MakeNewParticle(std::mt19937& randomEngine);

private:

    Mesh* mesh_ = nullptr;  // 非所有（MeshManagerが所有）

    uint32_t textureHandle_ = 0;

    // 全パーティクル共通のマテリアル（UpdateでmaterialCB_へ書き込む）
    Material material_{};

    Transform3D uvTransform_{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    ConstantBuffer<Material> materialCB_;

    // パーティクルの配列
    std::vector<Particle> particles_;

    // 乱数生成器（Initializeで初期化する。パーティクルの生成に使う）
    std::mt19937 randomEngine_;

    // Instancing用：パーティクルの数ぶんのワールド行列と色を格納するStructuredBuffer（VSのt0）
    StructuredBuffer<ParticleForGPU> instancingBuffer_;
};

} // namespace Engine
