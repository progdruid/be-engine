/*

@be-material: add-material {
    HDRInput: texture2d = black
    BloomInput: texture2d = black
    DirtTexture: texture2d = black
    InputSampler: sampler = linear-clamp
}

@be-shader bloom-add {
    topology triangle-strip

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s2 main add-material

    target s0 BloomOutput float3
}
*/

#include "fullscreen-vertex.hlsl"

Texture2D HDRInput : register(t0);
Texture2D BloomInput : register(t1);
Texture2D DirtTexture : register(t2);
SamplerState InputSampler : register(s0);

float3 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;
    float3 bloomColor = BloomInput.Sample(InputSampler, input.UV).rgb;
    float3 dirtColor = DirtTexture.Sample(InputSampler, input.UV).rgb;
    
    float dirt = dot(dirtColor, float3(0.333, 0.333, 0.333));
    
    float3 finalColor = hdrColor + bloomColor * (1.0 + dirt * 4.0);
    
    return finalColor;
}
