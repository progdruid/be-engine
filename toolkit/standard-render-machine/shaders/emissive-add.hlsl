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
#include "core/be-heap.hlsl"
#include "core/uniform-material.hlsl"

struct DrawRoot {
    uniform_material* Frame;
    uint InputEmissive;
    uint InputSampler;
};
[[vk::push_constant]] DrawRoot Root;

property uniform_material _Frame { get { return *Root.Frame; } }
property Texture2D InputEmissive { get { return Tex2DHeap[Root.InputEmissive]; } }
property SamplerState InputSampler { get { return SamplerHeap[Root.InputSampler]; } }

struct PixelOutput {
    float3 HDROutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "core/fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 emissiveColor = InputEmissive.Sample(InputSampler, input.UV).rgb;
    
    PixelOutput output;
    output.HDROutput = emissiveColor;
    return output;
}



