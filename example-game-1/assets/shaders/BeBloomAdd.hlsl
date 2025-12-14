/*
@be-shader-header
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "targets": {
        "BloomOutput": 0
    },
    "material": {
        "HDRInput": { "type": "texture2d", "slot": 0, "default": "black" },
        "BloomInput": { "type": "texture2d", "slot": 1, "default": "black" },
        "DirtTexture": { "type": "texture2d", "slot": 2, "default": "black" },
        "InputSampler": { "type": "sampler", "slot": 0 }
    }
}
@be-shader-header-end
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
