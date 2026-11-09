#include "Object3d.hlsli"

// オブジェクトごとのワールド行列（視点に依存しない）
struct TransformationMatrix
{
    float4x4 World;
    float4x4 WorldInverseTranspose; // 法線変換用（非均一スケール対応）
};

cbuffer TransformationMatrixBuffer : register(b0)
{
    TransformationMatrix gTransformationMatrix;
};

// 視点ごとのビュー射影行列（立体視で視点ごとに差し替える）
cbuffer ViewProjectionBuffer : register(b1)
{
    float4x4 gViewProjection;
};

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};
VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    float4 worldPosition = mul(input.position, gTransformationMatrix.World);
    output.position = mul(worldPosition, gViewProjection);
    output.texcoord = input.texcoord;

    // 法線はワールド行列の逆転置行列で変換する
    // （ワールド行列のままだと、非均一スケールで法線が面に垂直でなくなる）
    output.normal =
    normalize(
        mul(input.normal,
            (float3x3) gTransformationMatrix.WorldInverseTranspose)
    );
    output.worldPosition = worldPosition.xyz;
    return output;
}
