#include "Particle.hlsli"

// 全インスタンス共通のマテリアル（C++側 Engine::Material と一致させる。
// lightingModeはObject3dと構造体を共有するために残しているだけで、パーティクルでは使わない）
struct Material
{
    float4 color;
    int lightingMode;
    float3 padding;
    float4x4 uvTransform;
};

cbuffer MaterialBuffer : register(b0)
{
    Material material;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// かんたんのためライティングはしない（マテリアルの色×テクスチャの色をそのまま出す）
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 transformedUV =
        mul(
            float4(input.texcoord, 0.0f, 1.0f),
            material.uvTransform
        );

    float4 textureColor =
        gTexture.Sample(
            gSampler,
            transformedUV.xy
        );

    output.color = material.color * textureColor;

    // 最終的なαが0のときだけ描画しない
    if (output.color.a == 0.0f)
    {
        discard;
    }

    return output;
}
