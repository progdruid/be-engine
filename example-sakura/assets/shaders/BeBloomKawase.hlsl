/*

@be-material: bloom-kawase-material
[
    "TexelSize: float2 = [0.001, 0.001]",
    "PassRadius: float = 0.5",

    "BloomMipInput: texture2d = black",
    "InputSampler: sampler = linear-clamp",
]
@be-end

@be-shader: bloom-kawase
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "rasterizer": "back-solid",
    "blend": "disable",
    "depthStencil": "disable",
    "materials": {
        "main": { "scheme": "bloom-kawase-material", "slot": 1 },
    },
    "targets": {
        "BloomMipOutput": { "type": "float3", "slot": 0 }
    },
}
@be-end

*/

#include "fullscreen-vertex.hlsl"

/*========================================================*/
// region @be-auto-boilerplate
struct bloom_kawase_material {
    float2 TexelSize;
    float PassRadius;
};

cbuffer CBuffer_1 : register(b0, space1) {
    bloom_kawase_material _Main;
};
SamplerState InputSampler : register(s1, space1);
Texture2D BloomMipInput : register(t2, space1);

struct PixelOutput {
    float3 BloomMipOutput : SV_Target0;
};

// endregion
/*========================================================*/

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float2 offset = _Main.TexelSize * _Main.PassRadius;

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
        color += BloomMipInput.Sample(InputSampler, sampleUV).rgb;
    }

    PixelOutput output;
    output.BloomMipOutput = color * 0.125;
    return output;
}
