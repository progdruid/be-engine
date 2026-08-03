/*

@be-material: backbuffer-material {
    InputTexture: texture2d = white
    InputSampler: sampler = point-clamp
}

@be-shader backbuffer {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s2 main backbuffer-material

    target s0 BackbufferColor float4
}

*/

/*========================================================*/
// region @be-auto-boilerplate
SamplerState InputSampler : register(s1, space2);
Texture2D InputTexture : register(t2, space2);

struct PixelOutput {
    float4 BackbufferColor : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 inputColor = InputTexture.Sample(InputSampler, input.UV).rgb;
    
    PixelOutput output;
    output.BackbufferColor = float4(inputColor, 1.f);
    return output;
}
