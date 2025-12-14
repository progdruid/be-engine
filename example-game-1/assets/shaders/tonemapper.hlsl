/*
@be-shader-header
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "targets": {
        "HDRTarget": 0
    },
    "material": {
        "HDRInput": { "type": "texture2d", "slot": 0, "default": "black" },
        "InputSampler": { "type": "sampler", "slot": 0 }
    }
}
@be-shader-header-end
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
