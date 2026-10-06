#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <dxcapi.h>

namespace Engine {

/// <summary>
/// RootSignature（標準・パーティクル用）と用途別PSOを一括生成・所有するレジストリ。
/// 描画側はGet(Pipeline::～)でPSOを取得するだけでよく、
/// シェーダーコンパイルやPSO生成の詳細を知る必要がない。
/// </summary>
class PipelineManager {
public:

    // 用途別のPSO（Initializeで一括生成する）
    enum class Pipeline {
        kStandard,  // 裏面カリング（通常の3Dオブジェクト・スプライト）
        kNoCull,    // カリング無効（内側から見る天球など）
        kLine,      // ライントポロジ・深度無効（選択オブジェクトの軸ギズモなど常に手前に描く線分）
        kParticle,  // Instancing（パーティクル）。パーティクル用RootSignatureと組で使う

        kCount,     // PSOの総数（enumの末尾に置くこと）
    };

    // 標準RootSignatureのパラメータ番号（SetGraphicsRoot～ の第1引数に使う）
    enum RootParameter : UINT {
        kRootMaterial = 0,          // b0 [PS] マテリアル
        kRootWorldTransform = 1,    // b0 [VS] ワールド行列
        kRootDirectionalLight = 2,  // b1 [PS] 平行光源
        kRootTexture = 3,           // t0 [PS] テクスチャ（DescriptorTable）
        kRootViewProjection = 4,    // b1 [VS] 視点ごとのビュー射影
        kRootPointLights = 5,       // b2 [PS] 点光源

        kRootParameterCount,        // パラメータ数（enumの末尾に置くこと）
    };

    // パーティクル用RootSignatureのパラメータ番号（ライティングしないため光源は持たない）
    enum ParticleRootParameter : UINT {
        kParticleRootMaterial = 0,        // b0 [PS] マテリアル（全インスタンス共通）
        kParticleRootInstancing = 1,      // t0 [VS] インスタンスごとのワールド行列（StructuredBuffer。DescriptorTable）
        kParticleRootTexture = 2,         // t0 [PS] テクスチャ（DescriptorTable）
        kParticleRootViewProjection = 3,  // b1 [VS] 視点ごとのビュー射影

        kParticleRootParameterCount,      // パラメータ数（enumの末尾に置くこと）
    };

    static PipelineManager* GetInstance();

    // 標準・パーティクル用シェーダーをコンパイルし、RootSignatureと全PSOを生成する
    // （ShaderCompiler::Initializeの後に呼ぶ）
    void Initialize(ID3D12Device* device);

    // 全PSOとRootSignatureを解放する（リソースリークチェックより前に呼ぶ）
    void Finalize();

    // 標準RootSignature（kParticle以外のPSO用）
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

    // パーティクル用RootSignature（kParticleのPSO用。パラメータ番号はParticleRootParameter）
    ID3D12RootSignature* GetParticleRootSignature() const { return particleRootSignature_.Get(); }

    ID3D12PipelineState* Get(Pipeline pipeline) const;

    // PSOと、それに合うプリミティブトポロジ（線分PSOならライン、それ以外は三角形）をまとめて設定する。
    // （RootSignatureは設定しない。kParticleはパーティクル用RootSignatureを設定してから使う）
    void SetPipeline(ID3D12GraphicsCommandList* commandList, Pipeline pipeline) const;

public:

    struct PipelineConfig {
        ID3D12Device* device = nullptr;
        ID3D12RootSignature* rootSignature = nullptr;
        D3D12_INPUT_LAYOUT_DESC inputLayout{};
        D3D12_BLEND_DESC blendDesc{};
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        Microsoft::WRL::ComPtr<IDxcBlob> vertexShader;
        Microsoft::WRL::ComPtr<IDxcBlob> pixelShader;
    };

    static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateGraphicsPipeline(const PipelineConfig& config);

    // このプロジェクト標準のRootSignatureを生成する
    // (b0:Material[PS], b0:Transform[VS], b1:Light[PS], t0:Texture[PS], s0:Sampler)
    static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* device);

    // パーティクル（Instancing）用のRootSignatureを生成する
    // (b0:Material[PS], t0:Instancing[VS], t0:Texture[PS], b1:ViewProjection[VS], s0:Sampler)
    static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateParticleRootSignature(ID3D12Device* device);

    // このプロジェクト標準のInputLayout/各種Stateで描画パイプラインを生成する。
    // cullModeで裏面カリングの挙動を変えられる（天球は内側から見るためNONEを指定する）。
    static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateStandardPipeline(
        ID3D12Device* device,
        ID3D12RootSignature* rootSignature,
        IDxcBlob* vertexShader,
        IDxcBlob* pixelShader,
        D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK);

    // 線分描画用パイプラインを生成する（軸ギズモなど常に手前に表示する線分向け）。
    // ライントポロジ・カリング無効・深度無効（他オブジェクトに隠れず常に手前に表示される）。
    static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateLinePipeline(
        ID3D12Device* device,
        ID3D12RootSignature* rootSignature,
        IDxcBlob* vertexShader,
        IDxcBlob* pixelShader);

private:

    PipelineManager() = default;

    ~PipelineManager() = default;

    PipelineManager(const PipelineManager&) = delete;

    PipelineManager& operator=(const PipelineManager&) = delete;

private:

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

    // Object3d用とは別に作って管理する（rootParameters[1]がCBVではなくStructuredBufferのSRVになる）
    Microsoft::WRL::ComPtr<ID3D12RootSignature> particleRootSignature_;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelines_[size_t(Pipeline::kCount)];
};

} // namespace Engine
