#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int lightingMode;
    float3 padding;
    float4x4 uvTransform;
};

// ライティングの計算方式（CPU側Engine::LightingModeと一致させる）
static const int kLightingModeNone = 0; // ライティングなし
static const int kLightingModeLambert = 1; // ランバート反射
static const int kLightingModeHalfLambert = 2; // ハーフランバート反射

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

// 鏡面反射モデル
static const int kSpecularModelPhong = 0;
static const int kSpecularModelBlinnPhong = 1;

cbuffer SpecularBuffer : register(b3)
{
    float3 gCameraPosition;
    float gShininess;

    float gSpecularStrength;
    int gSpecularModel;
    float2 gSpecularPadding;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// N・Lから拡散反射係数を求める。
// Lambert:負を切り捨ててそのまま / Half Lambert:0..1へ写して2乗（陰が柔らかい）
// （分岐先でreturnするとコンパイラがX4000警告を出すため、単一returnで書く）
float DiffuseFactor(float NdotL, int lightingMode)
{
    float factor;
    if (lightingMode == kLightingModeLambert)
    {
        factor = saturate(NdotL);
    }
    else
    {
        factor = saturate(pow(NdotL * 0.5f + 0.5f, 2.0f));
    }
    return factor;
}

float SpecularFactor(
    float3 normal,
    float3 lightDirection,
    float3 viewDirection)
{
    float specular = 0.0f;

    // 裏面や光が当たっていない面には鏡面反射を出さない
    float NdotL = dot(normal, lightDirection);

    if (NdotL > 0.0f)
    {
        if (gSpecularModel == kSpecularModelBlinnPhong)
        {
            // Blinn-Phong：光源方向と視線方向の中間ベクトル
            float3 halfVector =
                normalize(lightDirection + viewDirection);

            specular = pow(
                saturate(dot(normal, halfVector)),
                max(gShininess, 1.0f)
            );
        }
        else
        {
            // Phong：反射ベクトルと視線方向の内積
            float3 reflectDirection =
                reflect(-lightDirection, normal);

            specular = pow(
                saturate(dot(reflectDirection, viewDirection)),
                max(gShininess, 1.0f)
            );
        }
    }

    return specular;
}

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
    
    // 透明部分を描画しない
    if (textureColor.a < 0.8f)
    {
        discard;
    }

    if (material.lightingMode == kLightingModeNone)
    {
        output.color =
            material.color *
            textureColor;

        return output;
    }

    float3 normal = normalize(input.normal);
       
    // 表面からカメラへ向かう方向
    float3 viewDirection =
        normalize(gCameraPosition - input.worldPosition);

    // 各光源の鏡面反射を加算する
    float3 specularLight = float3(0.0f, 0.0f, 0.0f);

    // 各光源の拡散反射（Lambert/Half Lambertはマテリアルの方式に従う）を加算合成する
    float3 diffuseLight = float3(0.0f, 0.0f, 0.0f);

    // --- 平行光源 ---
    if (gDirectionalLight.enabled != 0)
    {
        float3 lightDirection =
        -normalize(gDirectionalLight.direction);

        float NdotL = dot(normal, lightDirection);

        float diffuseFactor =
        DiffuseFactor(NdotL, material.lightingMode);

        diffuseLight +=
        gDirectionalLight.color.rgb *
        diffuseFactor *
        gDirectionalLight.intensity;

        float specularFactor =
        SpecularFactor(
            normal,
            lightDirection,
            viewDirection
        );

        specularLight +=
        gDirectionalLight.color.rgb *
        gDirectionalLight.intensity *
        specularFactor *
        gSpecularStrength;
    }

    // --- 点光源 ---
    for (int i = 0; i < kMaxPointLightCount; ++i)
    {
        if (gPointLights[i].enabled == 0)
        {
            continue;
        }

        // 光源→表面の方向と距離
        float3 lightToSurface =
    input.worldPosition - gPointLights[i].position;

        float distance = length(lightToSurface);

        float3 direction =
    lightToSurface / max(distance, 0.0001f);

// 表面→光源
        float3 lightDirection = -direction;

// 半径に基づく減衰
        float attenuation =
    pow(
        saturate(
            1.0f -
            distance / max(gPointLights[i].radius, 0.0001f)
        ),
        max(gPointLights[i].decay, 0.0001f)
    );

        float NdotL = dot(normal, lightDirection);

        float diffuseFactor =
    DiffuseFactor(NdotL, material.lightingMode);

        diffuseLight +=
    gPointLights[i].color.rgb *
    diffuseFactor *
    gPointLights[i].intensity *
    attenuation;

// 鏡面反射
        float specularFactor =
    SpecularFactor(
        normal,
        lightDirection,
        viewDirection
    );

        specularLight +=
    gPointLights[i].color.rgb *
    gPointLights[i].intensity *
    attenuation *
    specularFactor *
    gSpecularStrength;
    }

    // 透明度はライティングの影響を受けない（マテリアル×テクスチャのみ）    
    // 拡散反射 + 鏡面反射
    output.color.rgb =
        material.color.rgb *
        textureColor.rgb *
        diffuseLight
        +
        textureColor.rgb *
        specularLight;

    output.color.a = material.color.a * textureColor.a;
    
    return output;
}
