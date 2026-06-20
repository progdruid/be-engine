/*

@be-material: skybox-material {
    ClampRadiance: float = 0
    Depth: texture2d = black
    EnvCubemap: textureCube = black-cube
    InputSampler: sampler = linear-clamp
}

@be-shader skybox {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main skybox-material

    target s0 SkyHDR float3
}

*/

#include "BeFunctions.hlsli"
#include "fullscreen-vertex.hlsl"

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct skybox_material {
    float ClampRadiance;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    skybox_material _Main;
};
SamplerState InputSampler : register(s1, space1);
Texture2D Depth : register(t2, space1);
TextureCube EnvCubemap : register(t3, space1);

struct PixelOutput {
    float3 SkyHDR : SV_Target0;
};

// endregion
/*========================================================*/

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float depth = Depth.Sample(InputSampler, input.UV).r;
    if (depth < 1.0) discard;

    float3 worldFar = ReconstructWorldPosition(input.UV, 1.0, _Frame.CameraInverseProjectionView);
    float3 dir = normalize(worldFar - _Frame.CameraPosition);

    float3 color = EnvCubemap.Sample(InputSampler, dir).rgb;

    float maxComp = max(color.r, max(color.g, color.b));
    if (_Main.ClampRadiance > 0.0 && maxComp > _Main.ClampRadiance) {
        color *= _Main.ClampRadiance / maxComp;
    }

    PixelOutput output;
    output.SkyHDR = color;
    return output;
}
