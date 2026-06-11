/*

@be-material: emissive-add-material {
    InputEmissive: texture2d = black
    InputSampler: sampler = point-clamp
}

@be-shader emissive-add {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main emissive-add-material

    target s0 HDROutput float3
}
*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

SamplerState InputSampler : register(s1, space1);
Texture2D InputEmissive : register(t2, space1);

struct PixelOutput {
    float3 HDROutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 emissiveColor = InputEmissive.Sample(InputSampler, input.UV).rgb;
    
    PixelOutput output;
    output.HDROutput = emissiveColor;
    return output;
}



