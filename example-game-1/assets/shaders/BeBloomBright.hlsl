/*
@be-shader-header
{
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "targets": {
        "BloomMip": 0
    },
    "material": {
        "Threshold": { "type": "float", "default": 0.75 },
        "Intensity": { "type": "float", "default": 17.0 },
        "Knee": { "type": "float", "default": 1.25 },

        "HDRInput": { "type": "texture2d", "slot": 0, "default": "black" }
    }
}
@be-shader-header-end
*/

#include "fullscreen-vertex.hlsl"

Texture2D HDRInput : register(t0);
SamplerState Linear : register(s0);

cbuffer MaterialConstants : register(b2) {
    float Threshold;
    float Intensity;
    float Knee;
};

float3 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float3 hdrColor = HDRInput.Sample(Linear, input.UV).rgb;

    float luminance = dot(hdrColor, float3(0.2126, 0.7152, 0.0722));
    float brightPart = saturate((luminance - Threshold) * rcp(max(luminance, 0.0001)));
    float kneeFactor = smoothstep(Threshold, Threshold + Knee, luminance);
    float3 brightColor = hdrColor * brightPart * kneeFactor * Intensity;
    
    return brightColor;
}
