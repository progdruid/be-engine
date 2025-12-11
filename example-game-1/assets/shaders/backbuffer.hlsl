/*
@be-shader-header
{
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

Texture2D InputTexture : register(t0);
SamplerState InputSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float4 PixelFunction(PSInput input) : SV_TARGET {
    float3 inputColor = InputTexture.Sample(InputSampler, input.UV).rgb;
    return float4(inputColor, 1.f);
}
