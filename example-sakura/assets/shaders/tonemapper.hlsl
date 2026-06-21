/*

@be-material: tonemapper-material {
    Exposure: float = 0.2
    Contrast: float = 1.90
    HDRInput: texture2d = black
    InputSampler: sampler = point-clamp
}

@be-shader tonemapper {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main tonemapper-material

    target s0 HDRTarget float3
}
*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct tonemapper_material {
    float Exposure;
    float Contrast;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    tonemapper_material _Main;
};
SamplerState InputSampler : register(s1, space1);
Texture2D HDRInput : register(t2, space1);

struct PixelOutput {
    float3 HDRTarget : SV_Target0;
};

// endregion
/*========================================================*/

#include <BeTonemappers.hlsli>
#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;

    float3 graded = ApplyContrast(hdrColor * _Main.Exposure, _Main.Contrast);
    float3 finalColor = LinearToSrgb(Tonemap_ACES_Knarkowicz(graded));
    
    PixelOutput output;
    output.HDRTarget = finalColor;
    return output;
}
