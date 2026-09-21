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
#include "core/be-bindless-tables.hlsl"
#include "core/uniform-material.hlsl"

struct batched_lights_material {
    float LightCount;
    float4 LightPositionRadius[64];
    float4 LightColorPower[64];
};

struct DrawRoot {
    uniform_material* Frame;
    batched_lights_material* Main;
    uint Depth;
    uint Albedo_RGB;
    uint WorldNormal_XYZ;
    uint ORM_RGB;
    uint InputSampler;
};
[[vk::push_constant]] DrawRoot Root;

property uniform_material* _Frame { get { return Root.Frame; } }
property batched_lights_material* _Main { get { return Root.Main; } }
property Texture2D Depth { get { return Tex2DTable[Root.Depth]; } }
property Texture2D Albedo_RGB { get { return Tex2DTable[Root.Albedo_RGB]; } }
property Texture2D WorldNormal_XYZ { get { return Tex2DTable[Root.WorldNormal_XYZ]; } }
property Texture2D ORM_RGB { get { return Tex2DTable[Root.ORM_RGB]; } }
property SamplerState InputSampler { get { return SamplerTable[Root.InputSampler]; } }

struct PixelOutput {
    float3 LightHDR : SV_Target0;
};

// endregion
/*========================================================*/

#include "light-common.hlsli"
#include "core/fullscreen-vertex.hlsl"

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
