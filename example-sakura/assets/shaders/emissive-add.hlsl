/*

@be-material: emissive-add-material
[
    "InputEmissive: texture2d = black",
    "InputSampler: sampler = point-clamp",
]
@be-end

@be-shader: emissive-add
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "rasterizer": "back-solid",
    "blend": "disable",
    "depthStencil": "disable",
    "materials": {
        "main": { "scheme": "emissive-add-material", "slot": 1 },
    },
    "targets": {
        "HDROutput": { "type": "float3", "slot": 0 }
    },
}
@be-end
*/

/*========================================================*/
// region @be-auto-boilerplate
SamplerState InputSampler : register(s1, space1);
Texture2D InputEmissive : register(t2, space1);

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



