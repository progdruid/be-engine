/*

@be-material: bloom-downsample-material {
    TexelSize: float2 = (0.001, 0.001)
    UseKaris: float = 0.0
    BloomMipInput: texture2d = black
    InputSampler: sampler = linear-clamp
}

@be-shader bloom-downsample {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main bloom-downsample-material

    target s0 BloomMipOutput float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct bloom_downsample_material {
    float2 TexelSize;
    float UseKaris;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    bloom_downsample_material _Main;
};
SamplerState InputSampler : register(s1, space1);
Texture2D BloomMipInput : register(t2, space1);

struct PixelOutput {
    float3 BloomMipOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

// COD/Jimenez 13-tap downsample, Karis average on the first mip to kill fireflies.

float KarisWeight(float3 c) {
    float luma = dot(c, float3(0.2126, 0.7152, 0.0722));
    return 1.0 / (1.0 + luma);
}

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float2 t  = _Main.TexelSize;
    float2 uv = input.UV;

    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    float3 a = BloomMipInput.SampleLevel(InputSampler, uv + t * float2(-2.0, -2.0), 0).rgb;
    float3 b = BloomMipInput.SampleLevel(InputSampler, uv + t * float2( 0.0, -2.0), 0).rgb;
    float3 c = BloomMipInput.SampleLevel(InputSampler, uv + t * float2( 2.0, -2.0), 0).rgb;
    float3 d = BloomMipInput.SampleLevel(InputSampler, uv + t * float2(-2.0,  0.0), 0).rgb;
    float3 e = BloomMipInput.SampleLevel(InputSampler, uv                         , 0).rgb;
    float3 f = BloomMipInput.SampleLevel(InputSampler, uv + t * float2( 2.0,  0.0), 0).rgb;
    float3 g = BloomMipInput.SampleLevel(InputSampler, uv + t * float2(-2.0,  2.0), 0).rgb;
    float3 h = BloomMipInput.SampleLevel(InputSampler, uv + t * float2( 0.0,  2.0), 0).rgb;
    float3 i = BloomMipInput.SampleLevel(InputSampler, uv + t * float2( 2.0,  2.0), 0).rgb;
    float3 j = BloomMipInput.SampleLevel(InputSampler, uv + t * float2(-1.0, -1.0), 0).rgb;
    float3 k = BloomMipInput.SampleLevel(InputSampler, uv + t * float2( 1.0, -1.0), 0).rgb;
    float3 l = BloomMipInput.SampleLevel(InputSampler, uv + t * float2(-1.0,  1.0), 0).rgb;
    float3 m = BloomMipInput.SampleLevel(InputSampler, uv + t * float2( 1.0,  1.0), 0).rgb;

    float3 color;
    if (_Main.UseKaris > 0.5) {
        // Five 2x2 boxes (4 corners + centre), each weighted by its Karis average.
        float3 box0 = (a + b + d + e) * 0.25;
        float3 box1 = (b + c + e + f) * 0.25;
        float3 box2 = (d + e + g + h) * 0.25;
        float3 box3 = (e + f + h + i) * 0.25;
        float3 box4 = (j + k + l + m) * 0.25;

        float w0 = 0.125 * KarisWeight(box0);
        float w1 = 0.125 * KarisWeight(box1);
        float w2 = 0.125 * KarisWeight(box2);
        float w3 = 0.125 * KarisWeight(box3);
        float w4 = 0.5   * KarisWeight(box4);

        color  = box0 * w0 + box1 * w1 + box2 * w2 + box3 * w3 + box4 * w4;
        color /= max(w0 + w1 + w2 + w3 + w4, 0.0001);
    } else {
        color  = e * 0.125;
        color += (a + c + g + i) * 0.03125;
        color += (b + d + f + h) * 0.0625;
        color += (j + k + l + m) * 0.125;
    }

    PixelOutput output;
    output.BloomMipOutput = color;
    return output;
}
