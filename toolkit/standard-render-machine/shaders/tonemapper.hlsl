/*

@be-material: tonemapper-material {
    Exposure: float = 0.2
    Contrast: float = 1.80
    HDRInput: texture2d = black
    InputSampler: sampler = point-clamp
}

@be-shader tonemapper {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main tonemapper-material

    target s0 HDRTarget float3
}
*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/be-heap.hlsl"
#include "core/uniform-material.hlsl"

struct tonemapper_material {
    float Exposure;
    float Contrast;
};

struct DrawRoot {
    uniform_material* Frame;
    tonemapper_material* Main;
    uint HDRInput;
    uint InputSampler;
};
[[vk::push_constant]] DrawRoot Root;

property uniform_material _Frame { get { return *Root.Frame; } }
property tonemapper_material _Main { get { return *Root.Main; } }
property Texture2D HDRInput { get { return Tex2DHeap[Root.HDRInput]; } }
property SamplerState InputSampler { get { return SamplerHeap[Root.InputSampler]; } }

struct PixelOutput {
    float3 HDRTarget : SV_Target0;
};

// endregion
/*========================================================*/

#include "BeTonemappers.hlsli"
#include "core/fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;

    float3 graded = ApplyContrast(hdrColor * _Main.Exposure, _Main.Contrast);
    float3 finalColor = LinearToSrgb(Tonemap_ACES_Knarkowicz(graded));

    PixelOutput output;
    output.HDRTarget = finalColor;
    return output;
}
