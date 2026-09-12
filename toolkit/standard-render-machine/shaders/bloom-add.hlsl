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
#include "core/be-heap.hlsl"
#include "core/uniform-material.hlsl"

struct DrawRoot {
    uniform_material* Frame;
    uint HDRInput;
    uint BloomInput;
    uint DirtTexture;
    uint InputSampler;
};
[[vk::push_constant]] DrawRoot Root;

property uniform_material* _Frame { get { return Root.Frame; } }
property Texture2D HDRInput { get { return Tex2DHeap[Root.HDRInput]; } }
property Texture2D BloomInput { get { return Tex2DHeap[Root.BloomInput]; } }
property Texture2D DirtTexture { get { return Tex2DHeap[Root.DirtTexture]; } }
property SamplerState InputSampler { get { return SamplerHeap[Root.InputSampler]; } }

struct PixelOutput {
    float3 BloomOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "core/fullscreen-vertex.hlsl"

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
