/*

@be-material: tonemapper-material
[
    "HDRInput: texture2d(0) = black",
    "InputSampler: sampler(0) = point-clamp",
]
@be-end

@be-shader: tonemapper
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "materials": {
        "main": { "scheme": "tonemapper-material", "slot": 2 },
    },
    "targets": {
        "HDRTarget": { "type": "float3", "slot": 0 }
    },
}
@be-end
*/

/*========================================================*/
// region @be-auto-boilerplate
Texture2D HDRInput : register(t0);
SamplerState InputSampler : register(s0);

struct PixelOutput {
    float3 HDRTarget : SV_Target0;
};

// endregion
/*========================================================*/

#include <BeTonemappers.hlsli>
#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;

    float3 finalColor = Tonemap_ReinhardWhite(hdrColor, 1.5);
    
    PixelOutput output;
    output.HDRTarget = finalColor;
    return output;
}
