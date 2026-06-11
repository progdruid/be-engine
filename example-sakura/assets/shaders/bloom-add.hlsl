/*

@be-material: bloom-add-material {
    HDRInput: texture2d = black
    BloomInput: texture2d = black
    DirtTexture: texture2d = black
    InputSampler: sampler = linear-clamp
}

@be-shader bloom-add {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main bloom-add-material

    target s0 BloomOutput float3
}
*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

SamplerState InputSampler : register(s1, space1);
Texture2D HDRInput : register(t2, space1);
Texture2D BloomInput : register(t3, space1);
Texture2D DirtTexture : register(t4, space1);

struct PixelOutput {
    float3 BloomOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;
    float3 bloomColor = BloomInput.Sample(InputSampler, input.UV).rgb;
    float3 dirtColor = DirtTexture.Sample(InputSampler, input.UV).rgb;
    
    float dirt = 0.0;//dot(dirtColor, float3(0.333, 0.333, 0.333));
    
    float3 finalColor = hdrColor + bloomColor * (1.0 + dirt * 6.0);
    
    PixelOutput output;
    output.BloomOutput = finalColor;
    return output;
}
