/*

@be-material: Main
{
    "HDRInput": "texture2d(0) = black",
    "InputSampler": "sampler(0)",
}
@be-end

@be-shader:
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "material": "Main",
    "targets": {
        "HDRTarget": 0
    },
}
@be-end
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
