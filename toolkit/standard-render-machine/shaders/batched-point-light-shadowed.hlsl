/*

@be-material: batched-point-light-shadowed-material {
    LightCount: float = 0
    LightPositionRadius: float4[32] = []
    LightColorPower: float4[32] = []
    LightShadowParams: float4[32] = []
    Depth: texture2d = black
    Albedo_RGB: texture2d = black
    WorldNormal_XYZ: texture2d = black
    ORM_RGB: texture2d = black
    PointLightShadowMap: textureCube[] = black-cube-array
    InputSampler: sampler = point-clamp
}

@be-shader batched-point-light-shadowed {
    topology triangle-strip
    rasterizer back-solid
    blend additive
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main batched-point-light-shadowed-material

    target s0 LightHDR float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/uniform-material.hlsl"

struct batched_point_light_shadowed_material {
    float LightCount;
    float4 LightPositionRadius[32];
    float4 LightColorPower[32];
    float4 LightShadowParams[32];
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    batched_point_light_shadowed_material _Main;
};
SamplerState InputSampler : register(s1, space1);
Texture2D Depth : register(t2, space1);
Texture2D Albedo_RGB : register(t3, space1);
Texture2D WorldNormal_XYZ : register(t4, space1);
Texture2D ORM_RGB : register(t5, space1);
TextureCubeArray PointLightShadowMap : register(t6, space1);

struct PixelOutput {
    float3 LightHDR : SV_Target0;
};

// endregion
/*========================================================*/

#include "light-common.hlsli"
#include "core/fullscreen-vertex.hlsl"

float SamplePointShadow(
    TextureCubeArray shadowMap, 
    SamplerState samp, 
    float3 worldPos, 
    float3 lightPos, 
    float radius, 
    float nearPlane, 
    float slice
) {
    float3 lightDir = worldPos - lightPos;
    float distanceToLight = length(lightDir);
    float3 sampleDir = normalize(lightDir);

    float shadowmapDepth = shadowMap.Sample(samp, float4(sampleDir, slice)).r;
    float near = nearPlane;
    float far = radius;
    float linearDepth = (near * far) / (far - shadowmapDepth * (far - near));

    float xCos = abs(dot(sampleDir, float3(1, 0, 0)));
    float yCos = abs(dot(sampleDir, float3(0, 1, 0)));
    float zCos = abs(dot(sampleDir, float3(0, 0, 1)));
    float cos = max(xCos, max(yCos, zCos));

    float shadowmapDistance = linearDepth / cos;
    return (distanceToLight - 0.02) < shadowmapDistance ? 1.0 : 0.0;
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
        float4 positionRadius = _Main.LightPositionRadius[i];
        float4 colorPower     = _Main.LightColorPower[i];
        float4 shadowParams   = _Main.LightShadowParams[i];

        float shadow = SamplePointShadow(
            PointLightShadowMap, InputSampler, worldPos,
            positionRadius.xyz, positionRadius.w, shadowParams.y, shadowParams.x
        );
        accumulated += ShadePointLight(
            surfacePoint, positionRadius.xyz, positionRadius.w,
            colorPower.rgb, colorPower.a
        ) * shadow;
    }

    PixelOutput output;
    output.LightHDR = accumulated;
    return output;
}
