/*

@be-material: emissive-add-material
[
    "InputEmissive: texture2d(0) = black",
    "InputSampler: sampler(0) = point-clamp",
]
@be-end

@be-shader: emissive-add
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "materials": {
        "main": { "scheme": "emissive-add-material", "slot": 2 },
    },
    "targets": {
        "HDROutput": { "type": "float3", "slot": 0 }
    },
}
@be-end
*/

/*========================================================*/
// region @be-auto-boilerplate
Texture2D InputEmissive : register(t0);
SamplerState InputSampler : register(s0);

struct PixelOutput {
    float3 HDROutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 emissiveColor = InputEmissive.Sample(InputSampler, input.UV).rgb;
    
    PixelOutput output;
    output.HDROutput = emissiveColor;
    return output;
}



