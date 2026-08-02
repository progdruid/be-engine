/*

@be-material: batched-lights-material {
    LightCount: float = 0
    LightPositionRadius: float4[64] = []
    LightColorPower: float4[64] = []
    Depth: texture2d = black
    Albedo_RGB: texture2d = black
    WorldNormal_XYZ: texture2d = black
    ORM_RGB: texture2d = black
    InputSampler: sampler = point-clamp
}

@be-shader batched-lights {
    topology triangle-strip
    rasterizer back-solid
    blend additive
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main batched-lights-material

    target s0 LightHDR float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct batched_lights_material {
    float LightCount;
    float4 LightPositionRadius[64];
    float4 LightColorPower[64];
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    batched_lights_material _Main;
};
SamplerState InputSampler : register(s1, space1);
Texture2D Depth : register(t2, space1);
Texture2D Albedo_RGB : register(t3, space1);
Texture2D WorldNormal_XYZ : register(t4, space1);
Texture2D ORM_RGB : register(t5, space1);

struct PixelOutput {
    float3 LightHDR : SV_Target0;
};

// endregion
/*========================================================*/

#include "light-common.hlsli"
#include "fullscreen-vertex.hlsl"

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
        float4 colorPower = _Main.LightColorPower[i];

        if (positionRadius.w > 0.0) {
            accumulated += ShadePointLight(
                surfacePoint, positionRadius.xyz, positionRadius.w,
                colorPower.rgb, colorPower.a
            );
        } else {
            accumulated += ShadeDirectionalLight(
                surfacePoint, positionRadius.xyz,
                colorPower.rgb, colorPower.a
            );
        }
    }

    PixelOutput output;
    output.LightHDR = accumulated;
    return output;
}
