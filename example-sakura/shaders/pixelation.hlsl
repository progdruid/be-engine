/*

@be-material: pixelation-material {
    ColorTexture: texture2d = white
    DepthTexture: texture2d = black
    PointSampler: sampler = point-clamp
    PixelSize: float = 8.0
    EdgeEnabled: float = 1.0
    EdgeThreshold: float = 1.0
    EdgeFarCutoff: float = 40.0
}

@be-shader pixelation {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PS

    bind s0 frame uniform-material
    bind s1 main pixelation-material

    target s0 PixelOutput float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/be-bindless-tables.hlsl"
#include "core/uniform-material.hlsl"

struct pixelation_material {
    float PixelSize;
    float EdgeEnabled;
    float EdgeThreshold;
    float EdgeFarCutoff;
};

struct DrawRoot {
    uniform_material* Frame;
    pixelation_material* Main;
    uint ColorTexture;
    uint DepthTexture;
    uint PointSampler;
};
[[vk::push_constant]] DrawRoot Root;

property uniform_material* _Frame { get { return Root.Frame; } }
property pixelation_material* _Main { get { return Root.Main; } }
property Texture2D ColorTexture { get { return Tex2DTable[Root.ColorTexture]; } }
property Texture2D DepthTexture { get { return Tex2DTable[Root.DepthTexture]; } }
property SamplerState PointSampler { get { return SamplerTable[Root.PointSampler]; } }

struct PixelOutput {
    float3 PixelOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "core/fullscreen-vertex.hlsl"

float LinearDepth(float d) {
    float near = _Frame.NearFarPlane.x;
    float far  = _Frame.NearFarPlane.y;
    return (near * far) / (far - d * (far - near));
}

PixelOutput PS(FullscreenVSOutput input) {
    uint w, h;
    ColorTexture.GetDimensions(w, h);

    float2 texel = 1.0 / float2(w, h);
    float2 blockUV = _Main.PixelSize * texel;
    float2 blockOrigin = floor(input.UV / blockUV) * blockUV;
    float2 snappedUV = blockOrigin + blockUV * 0.5;

    float3 color = ColorTexture.SampleLevel(PointSampler, snappedUV, 0).rgb;

    float2 o = texel * 0.5;
    float dTL = LinearDepth(DepthTexture.SampleLevel(PointSampler, blockOrigin + float2(-o.x,            -o.y),            0).r);
    float dTR = LinearDepth(DepthTexture.SampleLevel(PointSampler, blockOrigin + float2(blockUV.x + o.x, -o.y),            0).r);
    float dBL = LinearDepth(DepthTexture.SampleLevel(PointSampler, blockOrigin + float2(-o.x,            blockUV.y + o.y), 0).r);
    float dBR = LinearDepth(DepthTexture.SampleLevel(PointSampler, blockOrigin + float2(blockUV.x + o.x, blockUV.y + o.y), 0).r);

    float dMin = min(min(dTL, dTR), min(dBL, dBR));
    float dMax = max(max(dTL, dTR), max(dBL, dBR));
    float threshold = _Main.EdgeThreshold * ((_Main.PixelSize + 1.0) / 9.0);
    float edge = step(threshold, dMax - dMin) * step(dMin, _Main.EdgeFarCutoff) * step(0.5, _Main.EdgeEnabled);

    PixelOutput output;
    output.PixelOutput = lerp(color, float3(0, 0, 0), edge);
    return output;
}
