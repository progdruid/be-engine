/*

@be-material: kawase-material
[
    "TexelSize: float2 = [0.001, 0.001]",
    "PassRadius: float = 0.5",

    "BloomMipInput: texture2d(0) = black",
    "InputSampler: sampler(0) = linear-clamp",
]
@be-end

@be-shader: bloom-kawase
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "materials": {
        "main": { "scheme": "kawase-material", "slot": 2 },
    },
    "targets": {
        "BloomMipOutput": { "type": "float3", "slot": 0 }
    },
}
@be-end

*/

#include "fullscreen-vertex.hlsl"

Texture2D BloomMipInput : register(t0);
SamplerState sLinear : register(s0);

cbuffer MaterialConstants : register(b2) {
    float2 TexelSize;
    float PassRadius;
};

float3 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float2 offset = TexelSize * PassRadius;

    // Kawase filter: 4 cardinal + 4 diagonal directions (normalized)
    static const float2 offsets[8] = {
        float2( 1.0,  0.0),  // right
        float2(-1.0,  0.0),  // left
        float2( 0.0,  1.0),  // down
        float2( 0.0, -1.0),  // up
        float2( 0.7071,  0.7071),  // diagonal down-right
        float2(-0.7071,  0.7071),  // diagonal down-left
        float2( 0.7071, -0.7071),  // diagonal up-right
        float2(-0.7071, -0.7071)   // diagonal up-left
    };

    float3 color = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < 8; ++i) {
        float2 sampleUV = input.UV + offsets[i] * offset;
        color += BloomMipInput.Sample(sLinear, sampleUV).rgb;
    }

    return color * 0.125;
}
