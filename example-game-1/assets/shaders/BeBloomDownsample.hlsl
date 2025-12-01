/*
@be-shader-header
{
    "pixel": "PixelFunction",
    "material": {
        "TexelSize": { "type": "float2", "default": [0.001, 0.001] },
        "PassRadius": { "type": "float", "default": 0.5 },
        "MipLevel": { "type": "float", "default": 0.0 }
    }
}
@be-shader-header-end
*/

Texture2D Bloom : register(t0);
SamplerState sLinear : register(s0);

cbuffer MaterialConstants : register(b2) {
    float2 TexelSize;
    float PassRadius;
    float MipLevel;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float3 PixelFunction(PSInput input) : SV_TARGET {
    float2 offset = TexelSize * PassRadius;

    // kawase 4-tap spiral cone filter
    static const float2 offsets[4] = {
        float2( 0.0,      1.384),
        float2( 0.0,     -1.384),
        float2( 1.120,   -0.492),
        float2(-1.120,    0.492)
    };

    float3 color = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < 4; ++i) {
        float2 sampleUV = input.UV + offsets[i] * offset;
        color += Bloom.SampleLevel(sLinear, sampleUV, MipLevel).rgb;
    }

    return color * 0.25;
}
