/*

@be-material: backbuffer-material
[
    "InputTexture: texture2d(0) = white",
    "InputSampler: sampler(0) = point-clamp",
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
        "main": { "scheme": "backbuffer-material", "slot": 2 },
    },
    "targets": {
        "BackbufferColor": { "type": "float4", "slot": 0 }
    }
}
@be-end

*/

/*========================================================*/
// region @be-auto-boilerplate
Texture2D InputTexture : register(t0);
SamplerState InputSampler : register(s0);

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
