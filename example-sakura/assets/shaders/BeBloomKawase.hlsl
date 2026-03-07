/*

@be-material: bloom-kawase-material
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
    "rasterizer": "back-solid",
    "blend": "disable",
    "depthStencil": "disable",
    "materials": {
        "main": { "scheme": "bloom-kawase-material", "slot": 2 },
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

Texture2D BloomMipInput : register(t0);
SamplerState InputSampler : register(s0);

cbuffer CBuffer2 : register(b2) {
    bloom_kawase_material _Main;
};

struct PixelOutput {
    float3 BloomMipOutput : SV_Target0;
};

// endregion
/*========================================================*/

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float2 offset = _Main.TexelSize * _Main.PassRadius;
    
    static const float2 offsets[8] = {
        float2(-0.7, -0.7),  // top-left
        float2( 0.0, -1.0),  // top
        float2( 0.7, -0.7),  // top-right
        float2(-1.0,  0.0),  // left
        // float2(0.0, 0.0), // center - skip for ring blur
        float2( 1.0,  0.0),  // right
        float2(-0.7,  0.7),  // bottom-left
        float2( 0.0,  1.0),  // bottom
        float2( 0.7,  0.7)   // bottom-right
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
