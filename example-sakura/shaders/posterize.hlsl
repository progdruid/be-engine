/*

@be-material: posterize-material {
    ColorTexture: texture2d = white
    DepthTexture: texture2d = black
    UITexture: texture2d = black
    PointSampler: sampler = point-clamp
    PixelSize: float = 8.0
    DitherSpread: float = 0.5
    FogStart: float = 6.0
    FogEnd: float = 45.0
    FogColor: float3 = #334D80
    Enabled: float = 1.0
    Fade: float = 0.0
    PaletteCount: float = 7.0
    Palette: float3[8] = [#2E4372, #E89128, #F7F052, #D34E24, #8C3318, #1F2C47, #5D3B45]
}

@be-shader posterize {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PS

    bind s0 frame uniform-material
    bind s1 main posterize-material

    target s0 PosterizeOutput float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct posterize_material {
    float PixelSize;
    float DitherSpread;
    float FogStart;
    float FogEnd;
    float3 FogColor;
    float Enabled;
    float Fade;
    float PaletteCount;
    float3 Palette[8];
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    posterize_material _Main;
};
SamplerState PointSampler : register(s1, space1);
Texture2D ColorTexture : register(t2, space1);
Texture2D DepthTexture : register(t3, space1);
Texture2D UITexture : register(t4, space1);

struct PixelOutput {
    float3 PosterizeOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"
#include "BeFunctions.hlsli"

static const float BAYER8[64] = {
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
};

PixelOutput PS(FullscreenVSOutput input) {
    if (_Main.Enabled < 0.5) {
        float3 scene = ColorTexture.SampleLevel(PointSampler, input.UV, 0).rgb;
        float4 ui = UITexture.SampleLevel(PointSampler, input.UV, 0);
        PixelOutput passthrough;
        passthrough.PosterizeOutput = lerp(scene, ui.rgb, saturate(ui.a)) * (1.0 - _Main.Fade);
        return passthrough;
    }

    uint w, h;
    ColorTexture.GetDimensions(w, h);

    float2 texel = 1.0 / float2(w, h);
    float2 blockUV = _Main.PixelSize * texel;
    float2 blockOrigin = floor(input.UV / blockUV) * blockUV;
    float2 snappedUV = blockOrigin + blockUV * 0.5;

    float3 color = saturate(ColorTexture.SampleLevel(PointSampler, snappedUV, 0).rgb);

    float rawDepth = DepthTexture.SampleLevel(PointSampler, snappedUV, 0).r;
    float3 worldPos = ReconstructWorldPosition(snappedUV, rawDepth, _Frame.CameraInverseProjectionView);
    float dist = length(worldPos - _Frame.CameraPosition);
    float fog = saturate((dist - _Main.FogStart) / (_Main.FogEnd - _Main.FogStart));
    color = lerp(color, _Main.FogColor, fog);

    int2 blockIndex = int2(floor(input.UV / blockUV));
    float threshold = (BAYER8[(blockIndex.y & 7) * 8 + (blockIndex.x & 7)] + 0.5) / 64.0;

    int paletteCount = int(_Main.PaletteCount);
    float3 best = _Main.Palette[0];
    float3 second = _Main.Palette[0];
    float bestDist = 1e9;
    float secondDist = 1e9;
    [unroll]
    for (int i = 0; i < 8; i++) {
        if (i >= paletteCount) break;
        float3 delta = color - _Main.Palette[i];
        float dist = dot(delta, delta);
        if (dist < bestDist) {
            secondDist = bestDist;
            second = best;
            bestDist = dist;
            best = _Main.Palette[i];
        } else if (dist < secondDist) {
            secondDist = dist;
            second = _Main.Palette[i];
        }
    }

    float3 axis = second - best;
    float denom = dot(axis, axis);
    float t = denom > 1e-6 ? saturate(dot(color - best, axis) / denom) : 0.0;

    float strength = saturate(_Main.DitherSpread);
    float dithered = saturate(0.5 + (t - 0.5) / max(strength, 1e-4));

    float3 sceneOut = (threshold < dithered) ? second : best;

    float4 ui = UITexture.SampleLevel(PointSampler, input.UV, 0);
    PixelOutput output;
    output.PosterizeOutput = lerp(sceneOut, ui.rgb, saturate(ui.a)) * (1.0 - _Main.Fade);
    return output;
}
