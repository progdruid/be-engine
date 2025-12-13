/*
@be-shader-header
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "material": {
        "HasShadowMap": { "type": "float", "default": 0 },
        "Direction": { "type": "float3", "default": [0, 0, 0] },
        "Color": { "type": "float3", "default": [0, 0, 0] },
        "Power": { "type": "float", "default": 0 },
        "ProjectionView": { "type": "matrix" },
        "TexelSize": { "type": "float", "default": 0 },
        
        "Depth": { "type": "texture2d", "slot": 0, "default": "black" },
        "Diffuse": { "type": "texture2d", "slot": 1, "default": "black" },
        "WorldNormal": { "type": "texture2d", "slot": 2, "default": "black" },
        "Specular_Shininess": { "type": "texture2d", "slot": 3, "default": "black" },
        "ShadowMap": { "type": "texture2d", "slot": 4, "default": "black" }
    },
    "targets": {
        "LightHDR": 0
    }
}
@be-shader-header-end
*/

#include <BeUniformBuffer.hlsli>
#include <BeFunctions.hlsli>
#include "fullscreen-vertex.hlsl"

Texture2D Depth : register(t0);
Texture2D DiffuseRGB : register(t1);
Texture2D WorldNormalXYZ_UnusedA : register(t2);
Texture2D SpecularRGB_ShininessA : register(t3);
Texture2D DirectionalLightShadowMap : register(t4);
SamplerState InputSampler : register(s0);

cbuffer DirectionalLightBuffer: register(b2) {
    float _DirectionalLightHasShadowMap;
    float3 _DirectionalLightVector;
    float3 _DirectionalLightColor;
    float _DirectionalLightPower;
    
    row_major float4x4 _DirectionalLightProjectionView;
    float _ShadowMapTexelSize;
};

float PCFShadow(Texture2D shadowMap, SamplerState pcfSampler, float2 uv, float texelSize, float currentDepth) {
    float shadow = 0.0;

    // 3x3 PCF filter
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 sampleUV = uv + float2(x, y) * texelSize;
            float shadowmapDepth = shadowMap.Sample(pcfSampler, sampleUV).r;
            shadow += (currentDepth < shadowmapDepth + 0.001) ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

float3 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float depth = Depth.Sample(InputSampler, input.UV).r;
    float3 diffuse = DiffuseRGB.Sample(InputSampler, input.UV);
    float3 worldNormal = WorldNormalXYZ_UnusedA.Sample(InputSampler, input.UV).xyz;
    float4 specular_shininess = SpecularRGB_ShininessA.Sample(InputSampler, input.UV);

    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _CameraInverseProjectionView);

    float4 lightSpacePos = mul(float4(worldPos, 1.0), _DirectionalLightProjectionView);
    lightSpacePos /= lightSpacePos.w;
    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    float currentShadowDepth = lightSpacePos.z;
    float shadowAbsenceFactor = PCFShadow(DirectionalLightShadowMap, InputSampler, shadowUV, _ShadowMapTexelSize, currentShadowDepth);
    
    float3 viewVec = _CameraPosition - worldPos;
    float3 lit = StandardLambertBlinnPhong(
        worldNormal,
        viewVec,
        -_DirectionalLightVector,
        //_AmbientColor,
        _DirectionalLightColor,
        _DirectionalLightPower,
        diffuse.rgb,
        specular_shininess.rgb,
        specular_shininess.a
    );

    return lit * shadowAbsenceFactor;
}
