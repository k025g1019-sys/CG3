#include "Line.hlsli"

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 頂点の色をそのまま出す（ライティング・テクスチャなし）
PixelShaderOutput main(LineVertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = input.color;
    return output;
}
