/*

@be-material: smaa-blend-material
[
    "ColorTexture: texture2d = white",
    "EdgeTexture: texture2d = black",
    "LinearSampler: sampler = linear-clamp",
    "PointSampler: sampler = point-clamp",
]
@be-end

@be-shader: smaa-blend
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PS",
    "rasterizer": "back-solid",
    "blend": "disable",
    "depthStencil": "disable",
    "materials": {
        "frame": { "scheme": "uniform-material", "slot": 0 },
        "main": { "scheme": "smaa-blend-material", "slot": 1 },
    },
    "targets": {
        "AAOutput": { "type": "float3", "slot": 0 }
    }
}
@be-end

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

SamplerState LinearSampler : register(s1, space1);
SamplerState PointSampler : register(s2, space1);
Texture2D ColorTexture : register(t3, space1);
Texture2D EdgeTexture : register(t4, space1);

struct PixelOutput {
    float3 AAOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PS(FullscreenVSOutput input) {
    uint w, h;
    ColorTexture.GetDimensions(w, h);
    float2 texelSize = 1.0 / float2(w, h);
    float2 uv = input.UV;

    // Sample edges at this pixel and immediate neighbors
    float2 e  = EdgeTexture.Sample(PointSampler, uv).rg;
    float2 eR = EdgeTexture.Sample(PointSampler, uv + float2( texelSize.x, 0)).rg;
    float2 eB = EdgeTexture.Sample(PointSampler, uv + float2(0,  texelSize.y)).rg;

    // e.r  > 0: edge between this pixel and left neighbor  -> blend left
    // e.g  > 0: edge between this pixel and top neighbor   -> blend up
    // eR.r > 0: edge between right neighbor and this pixel -> blend right
    // eB.g > 0: edge between bottom neighbor and this pixel -> blend down
    //
    // Bilinear trick: sampling at uv +/- 0.5*texel uses the GPU's bilinear
    // filter to blend exactly 50% with the adjacent pixel in one sample.

    // Vertical axis: prefer blending up over down
    float2 vertOffset = float2(0, 0);
    if      (e.g  > 0.5) vertOffset = float2(0, -0.5 * texelSize.y);
    else if (eB.g > 0.5) vertOffset = float2(0,  0.5 * texelSize.y);

    // Horizontal axis: prefer blending left over right
    float2 horizOffset = float2(0, 0);
    if      (e.r  > 0.5) horizOffset = float2(-0.5 * texelSize.x, 0);
    else if (eR.r > 0.5) horizOffset = float2( 0.5 * texelSize.x, 0);

    bool hasVert  = dot(vertOffset,  vertOffset)  > 0;
    bool hasHoriz = dot(horizOffset, horizOffset) > 0;

    float3 color = ColorTexture.Sample(LinearSampler, uv).rgb;

    if (hasVert && hasHoriz) {
        // Corner: average center + vertical blend + horizontal blend
        float3 v = ColorTexture.Sample(LinearSampler, uv + vertOffset).rgb;
        float3 h = ColorTexture.Sample(LinearSampler, uv + horizOffset).rgb;
        color = (color + v + h) / 3.0;
    } else if (hasVert) {
        color = ColorTexture.Sample(LinearSampler, uv + vertOffset).rgb;
    } else if (hasHoriz) {
        color = ColorTexture.Sample(LinearSampler, uv + horizOffset).rgb;
    }

    PixelOutput o;
    o.AAOutput = color;
    return o;
}
