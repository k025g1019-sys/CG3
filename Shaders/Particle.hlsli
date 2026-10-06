struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;

    float3 normal : NORMAL0;
};

// パーティクルはライティングしないため、光源の構造体（DirectionalLight・PointLight）は持たない
