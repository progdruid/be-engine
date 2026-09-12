/*

@be-material: backbuffer-material {
    InputTexture: texture2d = white
    DepthTexture: texture2d = white
    DiscardFar: float = 0
    InputSampler: sampler = point-clamp
}

@be-shader backbuffer {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main backbuffer-material

    target s0 BackbufferColor float4
}


*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/be-heap.hlsl"
#include "core/uniform-material.hlsl"

struct backbuffer_material {
    float DiscardFar;
};

struct DrawRoot {
    uniform_material* Frame;
    backbuffer_material* Main;
    uint InputTexture;
    uint DepthTexture;
    uint InputSampler;
};
[[vk::push_constant]] DrawRoot Root;

property uniform_material* _Frame { get { return Root.Frame; } }
property backbuffer_material* _Main { get { return Root.Main; } }
property Texture2D InputTexture { get { return Tex2DHeap[Root.InputTexture]; } }
property Texture2D DepthTexture { get { return Tex2DHeap[Root.DepthTexture]; } }
property SamplerState InputSampler { get { return SamplerHeap[Root.InputSampler]; } }

struct PixelOutput {
    float4 BackbufferColor : SV_Target0;
};

// endregion
/*========================================================*/

#include "core/fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    if (_Main.DiscardFar > 0.5) {
        float depth = DepthTexture.Sample(InputSampler, input.UV).r;
        if (depth >= 0.9999) {
            discard;
        }
    }

    float3 inputColor = InputTexture.Sample(InputSampler, input.UV).rgb;

    PixelOutput output;
    output.BackbufferColor = float4(inputColor, 1.f);
    return output;
}
