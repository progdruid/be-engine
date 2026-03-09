/*

@be-material: bloom-add-material
[
    "HDRInput: texture2d(0) = black",
    "BloomInput: texture2d(1) = black",
    "DirtTexture: texture2d(2) = black",
    "InputSampler: sampler(0) = linear-clamp",
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
        "main": { "scheme": "bloom-add-material", "slot": 2 },
    },
    "targets": {
        "BloomOutput": { "type": "float3", "slot": 0 }
    },
}
@be-end
*/

/*========================================================*/
// region @be-auto-boilerplate
Texture2D HDRInput : register(t0);
Texture2D BloomInput : register(t1);
Texture2D DirtTexture : register(t2);
SamplerState InputSampler : register(s0);

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
