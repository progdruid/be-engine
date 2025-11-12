/*
@be-shader-header
{
    "pixel": "main"
}
@be-shader-header-end
*/

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

Texture2D inputTexture : register(t0);
SamplerState inputSampler : register(s0);

float3 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;
    float3 finalColor = inputTexture.Sample(inputSampler, uv).rgb;
    return finalColor;
}