/*
@be-shader-header
{
    "pixel": "PixelFunction"
}
@be-shader-header-end
*/

Texture2D HDRInput : register(t0);
Texture2D BloomInput : register(t1);
SamplerState InputSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float3 PixelFunction(PSInput input) : SV_TARGET {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;
    float3 bloomColor = BloomInput.Sample(InputSampler, input.UV).rgb;

    float3 finalColor = hdrColor + bloomColor;
    
    return finalColor;
}
