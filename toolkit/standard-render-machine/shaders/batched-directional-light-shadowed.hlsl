/*

@be-material: batched-directional-light-shadowed-material {
    LightCount: float = 0
    TexelSize: float = 0
    ShadowBias: float = 0.001
    
    LightDirection: float4[8] = []
    LightColorPower: float4[8] = []
    LightProjectionView: matrix[8]
    
    Depth: texture2d = black
    Albedo_RGB: texture2d = black
    WorldNormal_XYZ: texture2d = black
    ORM_RGB: texture2d = black
    ShadowMap: texture2d[] = black-array
    
    InputSampler: sampler = point-clamp
}

@be-shader batched-directional-light-shadowed {
    topology triangle-strip
    rasterizer back-solid
    blend additive
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main batched-directional-light-shadowed-material

    target s0 LightHDR float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct batched_directional_light_shadowed_material {
    float LightCount;
    float TexelSize;
    float ShadowBias;
    float4 LightDirection[8];
    float4 LightColorPower[8];
    float4x4 LightProjectionView[8];
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    batched_directional_light_shadowed_material _Main;
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

#include "light-common.hlsli"
#include "fullscreen-vertex.hlsl"

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
    float depth          = Depth.Sample(InputSampler, input.UV).r;
    float3 albedo        = Albedo_RGB.Sample(InputSampler, input.UV).rgb;
    float4 normalAndFlag = WorldNormal_XYZ.Sample(InputSampler, input.UV);
    float4 surface       = ORM_RGB.Sample(InputSampler, input.UV);

    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _Frame.CameraInverseProjectionView);
    BeSurfacePoint surfacePoint = MakeSurfacePoint(worldPos, _Frame.CameraPosition, normalAndFlag, albedo, surface);

    float3 accumulated = (0.0).xxx;
    int lightCount = int(_Main.LightCount);
    for (int i = 0; i < lightCount; ++i) {
        float4 directionSlice   = _Main.LightDirection[i];
        float4 colorPower       = _Main.LightColorPower[i];
        float4x4 projectionView = _Main.LightProjectionView[i];

        float shadowAbsenceFactor = 1.0;
        float4 lightSpacePos = mul(float4(worldPos, 1.0), projectionView);
        lightSpacePos /= lightSpacePos.w;
        float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
        shadowUV.y = 1.0 - shadowUV.y;
        float currentShadowDepth = lightSpacePos.z;
        
        bool insideShadowMap =
            all(shadowUV >= 0.0) && all(shadowUV <= 1.0) &&
            currentShadowDepth >= 0.0 && currentShadowDepth <= 1.0
        ;
        if (insideShadowMap) {
            shadowAbsenceFactor = PCFShadow(ShadowMap, directionSlice.w, InputSampler, shadowUV, _Main.TexelSize, currentShadowDepth, _Main.ShadowBias);
        }

        accumulated += ShadeDirectionalLight(
            surfacePoint, directionSlice.xyz, colorPower.rgb, colorPower.a
        ) * shadowAbsenceFactor;
    }

    PixelOutput output;
    output.LightHDR = accumulated;
    return output;
}
