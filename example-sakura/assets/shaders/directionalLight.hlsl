/*

@be-material: directional-light-material
[
    "HasShadowMap: float = 0",
    "Direction: float3 = [0, 0, 0]",
    "Color: float3 = [0, 0, 0]",
    "Power: float = 0",
    "ProjectionView: matrix",
    "TexelSize: float = 0",
    
    "Depth: texture2d(0) = black",
    "Diffuse: texture2d(1) = black",
    "WorldNormal: texture2d(2) = black",
    "Specular_Shininess: texture2d(3) = black",
    "ShadowMap: texture2d(4) = black",

    "InputSampler: sampler(0) = point-clamp",
]
@be-end

@be-shader: directional-light
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "rasterizer": "back-solid",
    "blend": "additive",
    "depthStencil": "disable",
    "materials": {
        "main": { "scheme": "directional-light-material", "slot": 1, "var": "DirectionalLight" },
    },
    "targets": {
        "LightHDR": { "type": "float3", "slot": 0 }
    }
}
@be-end

*/

#include <BeUniformBuffer.hlsli>
#include <BeFunctions.hlsli>
#include "fullscreen-vertex.hlsl"

/*========================================================*/
// region @be-auto-boilerplate
struct directional_light_material {
    float HasShadowMap;
    float3 Direction;
    float3 Color;
    float Power;
    float4x4 ProjectionView;
    float TexelSize;
};

Texture2D Depth : register(t0);
Texture2D Diffuse : register(t1);
Texture2D WorldNormal : register(t2);
Texture2D Specular_Shininess : register(t3);
Texture2D ShadowMap : register(t4);
SamplerState InputSampler : register(s0);

cbuffer CBuffer1 : register(b1) {
    directional_light_material _DirectionalLight;
};

struct PixelOutput {
    float3 LightHDR : SV_Target0;
};

// endregion
/*========================================================*/


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

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float depth = Depth.Sample(InputSampler, input.UV).r;
    float3 diffuse = Diffuse.Sample(InputSampler, input.UV).rgb;
    float3 worldNormal = WorldNormal.Sample(InputSampler, input.UV).xyz;
    float4 specular_shininess = Specular_Shininess.Sample(InputSampler, input.UV).rgba;

    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _CameraInverseProjectionView);

    float4 lightSpacePos = mul(float4(worldPos, 1.0), _DirectionalLight.ProjectionView);
    lightSpacePos /= lightSpacePos.w;
    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    float currentShadowDepth = lightSpacePos.z;
    float shadowAbsenceFactor = PCFShadow(ShadowMap, InputSampler, shadowUV, _DirectionalLight.TexelSize, currentShadowDepth);
    
    float3 viewVec = _CameraPosition - worldPos;
    float3 lit = StandardLambertBlinnPhong(
        worldNormal,
        viewVec,
        -_DirectionalLight.Direction,
        //_AmbientColor,
        _DirectionalLight.Color,
        _DirectionalLight.Power,
        diffuse.rgb,
        specular_shininess.rgb,
        specular_shininess.a
    );

    PixelOutput output;
    output.LightHDR = lit * shadowAbsenceFactor;
    return output;
}
