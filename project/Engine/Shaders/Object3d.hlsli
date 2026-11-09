struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;

    float3 normal : NORMAL0;

    // PointLightの距離・方向計算に使うワールド座標
    float3 worldPosition : POSITION0;
};
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    int enabled;
};
struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    int enabled;
    float padding;
};

// 点光源の最大数（C++側 Engine/Light/PointLight.h の kMaxPointLightCount と一致させる）
static const int kMaxPointLightCount = 4;
