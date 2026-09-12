/*

@be-material: bloom-bright-material {
    Threshold: float = 2.3
    Knee: float = 0.7
    Intensity: float = 0.7
    Clamp: float = 4.0
    HDRInput: texture2d = black
    InputSampler: sampler = linear-clamp
}


@be-shader bloom-bright {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main bloom-bright-material

    target s0 BloomMip float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/be-heap.hlsl"
#include "core/uniform-material.hlsl"

struct bloom_bright_material {
    float Threshold;
    float Knee;
    float Intensity;
    float Clamp;
};

struct DrawRoot {
    uniform_material* Frame;
    bloom_bright_material* Main;
    uint HDRInput;
    uint InputSampler;
};
[[vk::push_constant]] DrawRoot Root;

property uniform_material _Frame { get { return *Root.Frame; } }
property bloom_bright_material _Main { get { return *Root.Main; } }
property Texture2D HDRInput { get { return Tex2DHeap[Root.HDRInput]; } }
property SamplerState InputSampler { get { return SamplerHeap[Root.InputSampler]; } }

struct PixelOutput {
    float3 BloomMip : SV_Target0;
};

// endregion
/*========================================================*/

#include "core/fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;

    float brightness = dot(hdrColor, float3(0.2126, 0.7152, 0.0722));
    float knee = max(_Main.Knee, 0.0001);
    float soft = clamp(brightness - _Main.Threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee);
    float contribution = max(soft, brightness - _Main.Threshold) / max(brightness, 0.0001);
    float3 brightColor = hdrColor * contribution * _Main.Intensity;

    float maxComp = max(brightColor.r, max(brightColor.g, brightColor.b));
    brightColor *= 1.0 / max(1.0, maxComp / _Main.Clamp);

    PixelOutput output;
    output.BloomMip = brightColor;
    return output;
}
