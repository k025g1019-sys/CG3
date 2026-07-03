#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
};

cbuffer MaterialBuffer : register(b0)
{
    Material material;
};

cbuffer DirectionalLightBuffer : register(b1)
{
    DirectionalLight gDirectionalLight;
}

cbuffer PointLightBuffer : register(b2)
{
    PointLight gPointLights[kMaxPointLightCount];
}

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

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
    
    if (material.enableLighting == 0)
    {
        output.color =
            material.color *
            textureColor;

        return output;
    }
    
    float3 normal = normalize(input.normal);

    // 各光源の拡散反射（ハーフランバート）を加算合成する
    float3 diffuseLight = float3(0.0f, 0.0f, 0.0f);

    // --- 平行光源 ---
    if (gDirectionalLight.enabled != 0)
    {
        float NdotL =
            dot(
                normal,
                -normalize(gDirectionalLight.direction)
            );

        float cos =
        saturate(pow(NdotL * 0.5f + 0.5f, 2.0f));

        diffuseLight +=
            gDirectionalLight.color.rgb *
            cos *
            gDirectionalLight.intensity;
    }

    // --- 点光源 ---
    for (int i = 0; i < kMaxPointLightCount; ++i)
    {
        if (gPointLights[i].enabled == 0)
        {
            continue;
        }

        // 光源→画素の方向と距離（距離0でも0除算しないよう下限を設ける）
        float3 lightToSurface = input.worldPosition - gPointLights[i].position;
        float distance = length(lightToSurface);
        float3 direction = lightToSurface / max(distance, 0.0001f);

        // 半径の端で0になる減衰。radius/decayが0でもinf/NaNにならないよう下限を設ける
        float attenuation =
            pow(
                saturate(1.0f - distance / max(gPointLights[i].radius, 0.0001f)),
                max(gPointLights[i].decay, 0.0001f)
            );

        float NdotL = dot(normal, -direction);

        float cos =
        saturate(pow(NdotL * 0.5f + 0.5f, 2.0f));

        diffuseLight +=
            gPointLights[i].color.rgb *
            cos *
            gPointLights[i].intensity *
            attenuation;
    }

    // 透明度はライティングの影響を受けない（マテリアル×テクスチャのみ）
    output.color.rgb =
        material.color.rgb *
        textureColor.rgb *
        diffuseLight;
    output.color.a = material.color.a * textureColor.a;

    return output;
}