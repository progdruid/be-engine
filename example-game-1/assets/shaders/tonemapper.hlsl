/*

@be-material: tonemapper-material {
    HDRInput: texture2d = black
    InputSampler: sampler = point-clamp
}

@be-shader tonemapper {
    topology triangle-strip

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s2 main tonemapper-material

    target s0 HDRTarget float3
}
*/

#include <BeTonemappers.hlsli>
#include "fullscreen-vertex.hlsl"

Texture2D HDRInput : register(t0);
SamplerState InputSampler : register(s0);

float3 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;

    float3 finalColor = Tonemap_ReinhardWhite(hdrColor, 1.5);
    
    return finalColor;
}
