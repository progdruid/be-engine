/*

@be-material: backbuffer-material
[
    "InputTexture: texture2d = white",
    "InputSampler: sampler = point-clamp",
]
@be-end

@be-shader: backbuffer
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "rasterizer": "back-solid",
    "blend": "disable",
    "depthStencil": "disable",
    "materials": {
        "main": { "scheme": "backbuffer-material", "slot": 1 },
    },
    "targets": {
        "BackbufferColor": { "type": "float4", "slot": 0 }
    }
}
@be-end

*/

/*========================================================*/
// region @be-auto-boilerplate
SamplerState InputSampler : register(s1, space1);
Texture2D InputTexture : register(t2, space1);

struct PixelOutput {
    float4 BackbufferColor : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 inputColor = InputTexture.Sample(InputSampler, input.UV).rgb;
    
    PixelOutput output;
    output.BackbufferColor = float4(inputColor, 1.f);
    return output;
}
