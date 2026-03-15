/*

@be-material: bloom-add-material
[
    "HDRInput: texture2d = black",
    "BloomInput: texture2d = black",
    "DirtTexture: texture2d = black",
    "InputSampler: sampler = linear-clamp",
]
@be-end

@be-shader: bloom-add
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "rasterizer": "back-solid",
    "blend": "disable",
    "depthStencil": "disable",
    "materials": {
        "main": { "scheme": "bloom-add-material", "slot": 1 },
    },
    "targets": {
        "BloomOutput": { "type": "float3", "slot": 0 }
    },
}
@be-end
*/

/*========================================================*/
// region @be-auto-boilerplate
SamplerState InputSampler : register(s1, space1);
Texture2D HDRInput : register(t2, space1);
Texture2D BloomInput : register(t3, space1);
Texture2D DirtTexture : register(t4, space1);

struct PixelOutput {
    float3 BloomOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 hdrColor = HDRInput.Sample(InputSampler, input.UV).rgb;
    float3 bloomColor = BloomInput.Sample(InputSampler, input.UV).rgb;
    float3 dirtColor = DirtTexture.Sample(InputSampler, input.UV).rgb;
    
    float dirt = dot(dirtColor, float3(0.333, 0.333, 0.333));
    
    float3 finalColor = hdrColor + bloomColor * (1.0 + dirt * 6.0);
    
    PixelOutput output;
    output.BloomOutput = finalColor;
    return output;
}
