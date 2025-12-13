/*
@be-shader-header
{
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "material": {
        "InputTexture": { "type": "texture2d", "slot": 0, "default": "white" }
    },
    "targets": {
        "Backbuffer": 0
    }
}
@be-shader-header-end
*/

#include "fullscreen-vertex.hlsl"

Texture2D InputTexture : register(t0);
SamplerState InputSampler : register(s0);

float4 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float3 inputColor = InputTexture.Sample(InputSampler, input.UV).rgb;
    return float4(inputColor, 1.f);
}
