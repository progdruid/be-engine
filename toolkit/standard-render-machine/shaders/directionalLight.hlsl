/*

@be-material: directional-light-material {
    HasShadowMap: float = 0
    Direction: float3 = (0, 0, 0)
    Color: float3 = (0, 0, 0)
    Power: float = 0
    ProjectionView: matrix
    TexelSize: float = 0
    ShadowBias: float = 0.001
    Depth: texture2d = black
    Albedo_RGB: texture2d = black
    WorldNormal_XYZ: texture2d = black
    ORM_RGB: texture2d = black
    ShadowMap: texture2d[] = black-array
    ShadowSlice: float = 0
    InputSampler: sampler = point-clamp
}

@be-shader directional-light {
    topology triangle-strip
    rasterizer back-solid
    blend additive
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main directional-light-material DirectionalLight

    target s0 LightHDR float3
}

*/

#include "light-common.hlsli"
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
    float ShadowBias;
    float ShadowSlice;
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
Texture2DArray ShadowMap : register(t6, space1);

struct PixelOutput {
    float3 LightHDR : SV_Target0;
};

// endregion
/*========================================================*/


float PCFShadow(
    Texture2DArray shadowMap, 
    float slice, 
    SamplerState pcfSampler, 
    float2 uv, 
    float texelSize, 
    float currentDepth, 
    float bias
) {
    float shadow = 0.0;

    // 3x3 PCF filter
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 sampleUV = uv + float2(x, y) * texelSize;
            float shadowmapDepth = shadowMap.Sample(pcfSampler, float3(sampleUV, slice)).r;
            shadow += (currentDepth < shadowmapDepth + bias) ? 1.0 : 0.0;
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

    float shadowAbsenceFactor = 1.0;
    if (_DirectionalLight.HasShadowMap > 0.5) {
        float4 lightSpacePos = mul(float4(worldPos, 1.0), _DirectionalLight.ProjectionView);
        lightSpacePos /= lightSpacePos.w;
        float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
        shadowUV.y = 1.0 - shadowUV.y;
        float currentShadowDepth = lightSpacePos.z;
        bool insideShadowMap =
            all(shadowUV >= 0.0) && all(shadowUV <= 1.0) &&
            currentShadowDepth >= 0.0 && currentShadowDepth <= 1.0;
        if (insideShadowMap) {
            shadowAbsenceFactor = PCFShadow(ShadowMap, _DirectionalLight.ShadowSlice, InputSampler, shadowUV, _DirectionalLight.TexelSize, currentShadowDepth, _DirectionalLight.ShadowBias);
        }
    }

    BeSurfacePoint surfacePoint = MakeSurfacePoint(worldPos, _Frame.CameraPosition, normalAndFlag, albedo, surface);
    float3 lit = ShadeDirectionalLight(
        surfacePoint, _DirectionalLight.Direction,
        _DirectionalLight.Color, _DirectionalLight.Power
    );

    PixelOutput output;
    output.LightHDR = lit * shadowAbsenceFactor;
    return output;
}
