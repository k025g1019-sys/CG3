#pragma once

#include <cstdint>
#include <list>
#include <random>
#include <string>

#include "Engine/Math/Matrix4x4.h"
#include "Engine/Math/Vector4.h"
#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/StructuredBuffer.h"
#include "Engine/Rendering/TransformData3D.h"

namespace Engine {

class Mesh;
struct ParticleEmitter;

/// <summary>
/// 同じメッシュ（板ポリなど）を、パーティクルごとに別のTransform・色で、
/// 1回の描画命令（DrawInstanced）でまとめて描くパーティクル。
///   Initialize : メッシュと、パーティクルの最大数を決める（最初は1個も出ていない）
///   Emit       : エミッタ（ParticleEmitter）の設定で、パーティクルを発生させる
///   Update     : 移動・生存時間の更新をし、ワールド行列と色をStructuredBufferへ書き込む（毎フレーム、Drawより前に呼ぶ）
///                ビルボードが有効なら、渡されたビュー行列を使ってパーティクルの表を常にカメラへ向ける
///   Draw       : パーティクル用のRootSignature・PSO（加算合成）で描画する（シーンのOnDrawの中で呼ぶ）
/// パーティクルは発生時にエミッタの設定から位置・速度・色・生存時間を乱数で決め、生存時間に近づくほど薄くなる。
/// 生存時間を過ぎたものは消滅する。数が増減するため、パーティクルはstd::listで管理する。
/// パーティクルごとのワールド行列と色はStructuredBuffer（VSのt0）、ビュー射影は視点ごとの共有CBuffer（VSのb1）で渡す。
/// マテリアル（色・UV変換）とテクスチャは全パーティクル共通で、ライティングはしない。
/// </summary>
class ParticleSystem {
public:

    /// <summary>
    /// OBJファイルで初期化する（メッシュはMeshManagerが読み込み・共有する。テクスチャは最初のサブメッシュのmtl由来）
    /// </summary>
    /// <param name="objFilePath">実行ディレクトリからの相対パス（例: "resources/plane.obj"）</param>
    /// <param name="maxParticleCount">パーティクルの最大数（この数ぶんのGPUリソースを確保する。これを超えては発生・描画しない）</param>
    void Initialize(const std::string& objFilePath, uint32_t maxParticleCount);

    // エミッタからパーティクルを発生させる（エミッタの「1回あたりの発生数」ぶん。最大数を超えるぶんは発生させない）
    void Emit(const ParticleEmitter& emitter);

    // 出ているパーティクルをすべて消す
    void Clear() { particles_.clear(); }

    /// <summary>
    /// 各パーティクルを動かし、ワールド行列・色とマテリアルを書き込む（毎フレーム、Drawより前に呼ぶ）
    /// </summary>
    /// <param name="viewMatrix">カメラのビュー行列（ビルボードでパーティクルをカメラへ向けるために使う）</param>
    void Update(const Matrix4x4& viewMatrix);

    // 描画する（シーンのOnDrawの中で呼ぶ）。描き終わったら標準のRootSignature・PSOへ戻す。
    // パーティクルごとの視錐台カリングはしない（全パーティクルをまとめて描く）
    void Draw() const;

    // 今出ているパーティクルの数
    uint32_t GetParticleCount() const { return uint32_t(particles_.size()); }

    // パーティクルの最大数（Initializeで決めた数）
    uint32_t GetMaxParticleCount() const { return instancingBuffer_.GetElementCount(); }

    // パーティクルが最大数まで出ているか（trueの間は、Emitを呼んでも発生しない）
    bool IsFull() const { return GetParticleCount() >= GetMaxParticleCount(); }

    // ビルボード（パーティクルの表を常にカメラへ向ける）を使うか。既定は使う。
    // 使わないときは、各パーティクルのTransformの回転のままの向きで描く
    void SetBillboardEnabled(bool enabled) { isBillboardEnabled_ = enabled; }
    bool IsBillboardEnabled() const { return isBillboardEnabled_; }

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

    // パーティクルを1個生成する（位置・速度・色・生存時間を、エミッタの設定の範囲から乱数で決める）
    static Particle MakeNewParticle(std::mt19937& randomEngine, const ParticleEmitter& emitter);

private:

    Mesh* mesh_ = nullptr;  // 非所有（MeshManagerが所有）

    uint32_t textureHandle_ = 0;

    // 全パーティクル共通のマテリアル（UpdateでmaterialCB_へ書き込む）
    Material material_{};

    Transform3D uvTransform_{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    ConstantBuffer<Material> materialCB_;

    // 今出ているパーティクル（発生で末尾に足し、消滅で途中から抜くのでstd::listで持つ）
    std::list<Particle> particles_;

    // 描画するインスタンス数（UpdateでStructuredBufferへ書き込んだ数。最大数を超えない）
    uint32_t instanceCount_ = 0;

    // 乱数生成器（Initializeで初期化する。パーティクルの生成に使う）
    std::mt19937 randomEngine_;

    // ビルボードを使うか（trueなら、パーティクルの表が常にカメラを向く）
    bool isBillboardEnabled_ = true;

    // Instancing用：パーティクルの最大数ぶんのワールド行列と色を格納するStructuredBuffer（VSのt0）
    StructuredBuffer<ParticleForGPU> instancingBuffer_;
};

} // namespace Engine
