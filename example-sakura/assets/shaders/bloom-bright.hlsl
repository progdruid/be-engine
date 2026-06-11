/*

@be-material: bloom-bright-material {
    Threshold: float = 1.3
    Intensity: float = 1.8
    Knee: float = 1.0
    Clamp: float = 16.0
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
#include "uniform-material.hlsl"

struct bloom_bright_material {
    float Threshold;
    float Intensity;
    float Knee;
    float Clamp;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    bloom_bright_material _Main;
};
SamplerState InputSampler : register(s1, space1);
Texture2D HDRInput : register(t2, space1);

struct PixelOutput {
    float3 BloomMip : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb; // linear sampler

    // Unity soft-knee curve: quadratic ramp across [threshold-knee, threshold+knee],
    // hard linear cutoff above. Knee is the half-width of the soft region.
    float brightness = dot(hdrColor, float3(0.2126, 0.7152, 0.0722));
    float knee = max(_Main.Knee, 0.0001);
    float soft = clamp(brightness - _Main.Threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee);
    float contribution = max(soft, brightness - _Main.Threshold) / max(brightness, 0.0001);
    float3 brightColor = hdrColor * contribution * _Main.Intensity;

    // Clamp the bloom input so a near-mirror specular spike can't smear into a
    // giant glow disc. Scale by the brightest channel to preserve hue.
    float maxComp = max(brightColor.r, max(brightColor.g, brightColor.b));
    brightColor *= 1.0 / max(1.0, maxComp / _Main.Clamp);

    PixelOutput output;
    output.BloomMip = brightColor;
    return output;
}
