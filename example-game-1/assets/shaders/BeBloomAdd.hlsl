/*
@be-shader-header
{
    "pixel": "PixelFunction",
    "targets": {
        "BloomOutput": 0
    }
}
@be-shader-header-end
*/

Texture2D HDRInput : register(t0);
Texture2D BloomInput : register(t1);
Texture2D Dirt : register(t2);
SamplerState InputSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float3 PixelFunction(PSInput input) : SV_TARGET {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;
    float3 bloomColor = BloomInput.Sample(InputSampler, input.UV).rgb;
    float3 dirtColor = Dirt.Sample(InputSampler, input.UV).rgb;
    
    float dirt = dot(dirtColor, float3(0.333, 0.333, 0.333));
    
    float3 finalColor = hdrColor + bloomColor * (1.0 + dirt * 4.0);
    
    return finalColor;
}
