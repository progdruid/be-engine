/*

@be-material: backbuffer-material {
    InputTexture: texture2d = white
    InputSampler: sampler = point-clamp
}

@be-shader backbuffer {
    topology triangle-strip

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s2 main backbuffer-material

    target s0 Backbuffer float4
}

*/

#include "fullscreen-vertex.hlsl"

Texture2D InputTexture : register(t0);
SamplerState InputSampler : register(s0);

float4 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float3 inputColor = InputTexture.Sample(InputSampler, input.UV).rgb;
    return float4(inputColor, 1.f);
}
