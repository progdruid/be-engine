/*

@be-material: directional-light-material
[
    "HasShadowMap: float = 0",
    "Direction: float3 = [0, 0, 0]",
    "Color: float3 = [0, 0, 0]",
    "Power: float = 0",
    "ProjectionView: matrix",
    "TexelSize: float = 0",
    
    "Depth: texture2d = black",
    "Albedo_RGB: texture2d = black",
    "WorldNormal_XYZ: texture2d = black",
    "ORM_RGB: texture2d = black",
    "ShadowMap: texture2d = black",

    "InputSampler: sampler = point-clamp",
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
        "frame": { "scheme": "uniform-material", "slot": 0 },
        "main": { "scheme": "directional-light-material", "slot": 1, "var": "DirectionalLight" },
    },
    "targets": {
        "LightHDR": { "type": "float3", "slot": 0 }
    }
}
@be-end

*/

#include <BeFunctions.hlsli>
#include "fullscreen-vertex.hlsl"

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct directional_light_material {
    float HasShadowMap;
    float3 Direction;
    float3 Color;
    float Power;
    float4x4 ProjectionView;
    float TexelSize;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    directional_light_material _DirectionalLight;
};
SamplerState InputSampler : register(s1, space1);
Texture2D Depth : register(t2, space1);
Texture2D Albedo_RGB : register(t3, space1);
Texture2D WorldNormal_XYZ : register(t4, space1);
Texture2D ORM_RGB : register(t5, space1);
Texture2D ShadowMap : register(t6, space1);

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
    float depth             = Depth.Sample(InputSampler, input.UV).r;
    float3 albedo           = Albedo_RGB.Sample(InputSampler, input.UV).rgb;
    float4 normalAndFlag    = WorldNormal_XYZ.Sample(InputSampler, input.UV);
    float4 surface          = ORM_RGB.Sample(InputSampler, input.UV);

    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _Frame.CameraInverseProjectionView);

    float4 lightSpacePos = mul(float4(worldPos, 1.0), _DirectionalLight.ProjectionView);
    lightSpacePos /= lightSpacePos.w;
    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    float currentShadowDepth = lightSpacePos.z;
    float shadowAbsenceFactor = PCFShadow(ShadowMap, InputSampler, shadowUV, _DirectionalLight.TexelSize, currentShadowDepth);

    float3 viewVec = _Frame.CameraPosition - worldPos;
    float3 lit;
    if (normalAndFlag.w > 0.5) {
        lit = StandardLambertBlinnPhong(
            normalAndFlag.xyz, viewVec, -_DirectionalLight.Direction,
            _DirectionalLight.Color, _DirectionalLight.Power,
            albedo, surface.rgb, surface.a
        );
    } else {
        lit = StandardPBR(
            normalAndFlag.xyz, viewVec, -_DirectionalLight.Direction,
            _DirectionalLight.Color * _DirectionalLight.Power,
            albedo, surface.rgb
        );
    }

    PixelOutput output;
    output.LightHDR = lit * shadowAbsenceFactor;
    return output;
}
