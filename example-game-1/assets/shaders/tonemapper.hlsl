/*
@be-shader-header
{
    "pixel": "PixelFunction"
}
@be-shader-header-end
*/

#include <BeTonemappers.hlsli>

Texture2D HDRInput : register(t0);
SamplerState InputSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float3 PixelFunction(PSInput input) : SV_TARGET {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;

    float3 finalColor = Tonemap_ReinhardWhite(hdrColor, 1.0);
    
    return finalColor;
}
