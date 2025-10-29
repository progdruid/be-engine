#include <BeTonemappers.hlsli>
#include <BeUniformBuffer.hlsli>

Texture2D Depth : register(t0);
Texture2D DiffuseRGBA : register(t1);
Texture2D WorldNormalXYZ_UnusedA : register(t2);
Texture2D SpecularRGB_ShininessA : register(t3);
Texture2D Lightmap : register(t4);
SamplerState InputSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float depth = Depth.Sample(InputSampler, input.UV).r;
    float3 lightmapColor = Lightmap.Sample(InputSampler, input.UV).rgb;
    
    float3 finalColor = _AmbientColor + lightmapColor;
    
    finalColor = Tonemap_ReinhardWhite(finalColor, 2.0);
    
    return float4(finalColor, 1.f);
}
