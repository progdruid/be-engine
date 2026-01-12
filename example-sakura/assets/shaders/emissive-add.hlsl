/*

@be-material: emissive-add-material
[
    "InputEmissive: texture2d(0) = black",
    "InputSampler: sampler(0) = point-clamp",
]
@be-end

@be-shader: emissive-add
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "materials": {
        "main": { "scheme": "emissive-add-material", "slot": 2 },
    },
    "targets": {
        "HDROutput": 0
    },
}
@be-end
*/

#include "fullscreen-vertex.hlsl"

Texture2D InputEmissive : register(t0);
SamplerState InputSampler : register(s0);

float3 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float3 emissiveColor = InputEmissive.Sample(InputSampler, input.UV).rgb;
    return emissiveColor;
}
