#include "Line.hlsli"

// 視点ごとのビュー射影行列（立体視で視点ごとに差し替える）
cbuffer ViewProjectionBuffer : register(b0)
{
    float4x4 gViewProjection;
};

struct VertexShaderInput
{
    float3 position : POSITION0;  // ワールド座標
    float4 color : COLOR0;
};

LineVertexShaderOutput main(VertexShaderInput input)
{
    LineVertexShaderOutput output;
    output.position = mul(float4(input.position, 1.0f), gViewProjection);
    output.color = input.color;
    return output;
}
