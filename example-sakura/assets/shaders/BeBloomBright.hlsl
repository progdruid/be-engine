/*

@be-material: bloom-bright-material
[
    "Threshold: float = 0.9",
    "Intensity: float = 10.0",
    "Knee: float = 1.25",

    "HDRInput: texture2d(0) = black",
    "InputSampler: sampler(0) = linear-clamp",
]
@be-end


@be-shader: bloom-bright
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "materials": {
        "main": { "scheme": "bloom-bright-material", "slot": 2 },
    },
    "targets": {
        "BloomMip": { "type": "float3", "slot": 0 }
    },
}
@be-end

*/

/*========================================================*/
// region @be-auto-boilerplate
struct bloom_bright_material {
    float Threshold;
    float Intensity;
    float Knee;
};

Texture2D HDRInput : register(t0);
SamplerState InputSampler : register(s0);

cbuffer CBuffer2 : register(b2) {
    bloom_bright_material _Main;
};

struct PixelOutput {
    float3 BloomMip : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb; // linear sampler

    float luminance = dot(hdrColor, float3(0.2126, 0.7152, 0.0722));
    float brightPart = saturate((luminance - _Main.Threshold) * rcp(max(luminance, 0.0001)));
    float kneeFactor = smoothstep(_Main.Threshold, _Main.Threshold + _Main.Knee, luminance);
    float3 brightColor = hdrColor * brightPart * kneeFactor * _Main.Intensity;
    
    PixelOutput output;
    output.BloomMip = brightColor;
    return output;
}
