/*

@be-material: bloom-upsample-material {
    TexelSize: float2 = (0.001, 0.001)
    Radius: float = 1.0
    BloomMipInput: texture2d = black
    InputSampler: sampler = linear-clamp
}

@be-shader bloom-upsample {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main bloom-upsample-material

    target s0 BloomMipOutput float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct bloom_upsample_material {
    float2 TexelSize;
    float Radius;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    bloom_upsample_material _Main;
};
SamplerState InputSampler : register(s1, space1);
Texture2D BloomMipInput : register(t2, space1);

struct PixelOutput {
    float3 BloomMipOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

// 3x3 tent upsample, additively blended up the pyramid. Spread is fixed in source texels.

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float2 o  = _Main.TexelSize * _Main.Radius;
    float2 uv = input.UV;

    float3 a = BloomMipInput.Sample(InputSampler, uv + float2(-o.x,  o.y)).rgb;
    float3 b = BloomMipInput.Sample(InputSampler, uv + float2( 0.0,  o.y)).rgb;
    float3 c = BloomMipInput.Sample(InputSampler, uv + float2( o.x,  o.y)).rgb;
    float3 d = BloomMipInput.Sample(InputSampler, uv + float2(-o.x,  0.0)).rgb;
    float3 e = BloomMipInput.Sample(InputSampler, uv                    ).rgb;
    float3 f = BloomMipInput.Sample(InputSampler, uv + float2( o.x,  0.0)).rgb;
    float3 g = BloomMipInput.Sample(InputSampler, uv + float2(-o.x, -o.y)).rgb;
    float3 h = BloomMipInput.Sample(InputSampler, uv + float2( 0.0, -o.y)).rgb;
    float3 i = BloomMipInput.Sample(InputSampler, uv + float2( o.x, -o.y)).rgb;

    float3 color = e * 4.0;
    color += (b + d + f + h) * 2.0;
    color += (a + c + g + i);
    color *= (1.0 / 16.0);

    PixelOutput output;
    output.BloomMipOutput = color;
    return output;
}
