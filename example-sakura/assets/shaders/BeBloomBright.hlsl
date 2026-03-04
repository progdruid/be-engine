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

struct PixelOutput {
    float3 BloomMip : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

cbuffer MaterialConstants : register(b2) {
    bloom_bright_material _Material;
};

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb; // linear sampler

    float luminance = dot(hdrColor, float3(0.2126, 0.7152, 0.0722));
    float brightPart = saturate((luminance - _Material.Threshold) * rcp(max(luminance, 0.0001)));
    float kneeFactor = smoothstep(_Material.Threshold, _Material.Threshold + _Material.Knee, luminance);
    float3 brightColor = hdrColor * brightPart * kneeFactor * _Material.Intensity;
    
    PixelOutput output;
    output.BloomMip = brightColor;
    return output;
}
