/*

@be-material: smaa-edge-material {
    ColorTexture: texture2d = white
    ColorSampler: sampler = point-clamp
}

@be-shader smaa-edge {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PS

    bind s0 frame uniform-material
    bind s1 main smaa-edge-material

    target s0 Edges float4
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

SamplerState ColorSampler : register(s1, space1);
Texture2D ColorTexture : register(t2, space1);

struct PixelOutput {
    float4 Edges : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

#define SMAA_THRESHOLD 0.0001

float Luma(float3 rgb) {
    return dot(rgb, float3(0.2126, 0.7152, 0.0722));
}

PixelOutput PS(FullscreenVSOutput input) {
    uint w, h;
    ColorTexture.GetDimensions(w, h);
    float2 texelSize = 1.0 / float2(w, h);
    float2 uv = input.UV;

    float L     = Luma(ColorTexture.Sample(ColorSampler, uv).rgb);
    float Lleft = Luma(ColorTexture.Sample(ColorSampler, uv + float2(-1,  0) * texelSize).rgb);
    float Ltop  = Luma(ColorTexture.Sample(ColorSampler, uv + float2( 0, -1) * texelSize).rgb);

    // R = edge to the left (vertical geometry boundary), G = edge above (horizontal geometry boundary)
    float2 edges = step(SMAA_THRESHOLD, abs(L - float2(Lleft, Ltop)));

    PixelOutput o;
    o.Edges = float4(edges, 0, 0);
    return o;
}
