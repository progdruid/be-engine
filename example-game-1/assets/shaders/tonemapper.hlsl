/*

@be-material: tonemapper-material
[
    "HDRInput: texture2d = black",
    "InputSampler: sampler = point-clamp",
]
@be-end

@be-shader: tonemapper
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "materials": {
        "main": { "scheme": "tonemapper-material", "slot": 2 },
    },
    "targets": {
        "HDRTarget": { "type": "float3", "slot": 0 }
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
