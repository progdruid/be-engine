/*

@be-material: Main
{
    "InputTexture": "texture2d(0) = white",
    "InputSampler": "sampler(0)",
}
@be-end

@be-shader:
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "materials": {
        "Main": 2,
    },
    "targets": {
        "Backbuffer": 0
    }
}
@be-end

*/

#include "fullscreen-vertex.hlsl"

Texture2D InputTexture : register(t0);
SamplerState InputSampler : register(s0);

float4 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float3 inputColor = InputTexture.Sample(InputSampler, input.UV).rgb;
    return float4(inputColor, 1.f);
}
