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
#include "core/be-heap.hlsl"

struct DrawRoot {
    uint InputTexture;
    uint InputSampler;
};
[[vk::push_constant]] DrawRoot Root;

property Texture2D InputTexture { get { return Tex2DHeap[Root.InputTexture]; } }
property SamplerState InputSampler { get { return SamplerHeap[Root.InputSampler]; } }

struct PixelOutput {
    float4 BackbufferColor : SV_Target0;
};

// endregion
/*========================================================*/

#include "core/fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 inputColor = InputTexture.Sample(InputSampler, input.UV).rgb;
    
    PixelOutput output;
    output.BackbufferColor = float4(inputColor, 1.f);
    return output;
}
