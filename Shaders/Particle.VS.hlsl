#include "Particle.hlsli"

// インスタンスごとのワールド行列（視点に依存しない。C++側 Engine::TransformationMatrix と一致させる）
struct TransformationMatrix
{
    float4x4 World;
};

// 全インスタンス分のワールド行列（C++の配列のようにインデックスで読む）。
// StructuredBufferはViewとしてはSRVなので、register識別子はtを使う
StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

// 視点ごとのビュー射影行列（立体視で視点ごとに差し替える。全インスタンス共通）
cbuffer ViewProjectionBuffer : register(b1)
{
    float4x4 gViewProjection;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};
// instanceId：今処理しているインスタンスの番号（0 ～ instanceCount-1。SV_InstanceIDはシステムが設定する）
VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    // インスタンスごとに異なるTransform情報で描画する
    float4 worldPosition = mul(input.position, gTransformationMatrices[instanceId].World);
    output.position = mul(worldPosition, gViewProjection);
    output.texcoord = input.texcoord;

    output.normal =
    normalize(
        mul(input.normal,
            (float3x3) gTransformationMatrices[instanceId].World)
    );
    return output;
}
