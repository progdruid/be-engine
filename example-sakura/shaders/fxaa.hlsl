/*

@be-material: fxaa-material {
    ColorTexture: texture2d = white
    LinearSampler: sampler = linear-clamp
}

@be-shader fxaa {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PS

    bind s0 frame uniform-material
    bind s1 main fxaa-material

    target s0 AAOutput float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

SamplerState LinearSampler : register(s1, space1);
Texture2D ColorTexture : register(t2, space1);

struct PixelOutput {
    float3 AAOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

// ---------------------------------------------------------------------------
// FXAA 3.11 — Timothy Lottes (NVIDIA)
// Quality preset: medium-high (12 search steps with variable step sizes)
// ---------------------------------------------------------------------------

#define FXAA_EDGE_THRESHOLD     (1.0 / 8.0)   // min contrast to be an edge
#define FXAA_EDGE_THRESHOLD_MIN (1.0 / 24.0)  // ignore dark pixels
#define FXAA_SUBPIX             0.75           // subpixel blend strength
#define FXAA_SUBPIX_TRIM        0.25           // subpixel blend cutoff
#define FXAA_SUBPIX_CAP         0.875          // subpixel blend cap

// Step sizes for edge search (12 steps, increasing for quality/perf balance)
static const float SEARCH_STEPS[12] = {
    1.0, 1.0, 1.0, 1.0,
    1.0, 1.5, 2.0, 2.0,
    2.0, 2.0, 4.0, 8.0
};

// ---------------------------------------------------------------------------

float FxaaLuma(float3 rgb) {
    // Green-biased luma — fast and sufficient for edge detection
    return rgb.y * (0.587 / 0.299) + rgb.x;
}

float3 FxaaSample(float2 uv) {
    return ColorTexture.SampleLevel(LinearSampler, uv, 0).rgb;
}

// ---------------------------------------------------------------------------

float3 Fxaa(float2 uv, float2 rcpFrame) {
    // -----------------------------------------------------------------------
    // 1. Sample 3x3 neighborhood lumas
    // -----------------------------------------------------------------------
    float3 rgbNW = FxaaSample(uv + float2(-1, -1) * rcpFrame);
    float3 rgbNE = FxaaSample(uv + float2( 1, -1) * rcpFrame);
    float3 rgbSW = FxaaSample(uv + float2(-1,  1) * rcpFrame);
    float3 rgbSE = FxaaSample(uv + float2( 1,  1) * rcpFrame);
    float3 rgbM  = FxaaSample(uv);

    float lumaNW = FxaaLuma(rgbNW);
    float lumaNE = FxaaLuma(rgbNE);
    float lumaSW = FxaaLuma(rgbSW);
    float lumaSE = FxaaLuma(rgbSE);
    float lumaM  = FxaaLuma(rgbM);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // -----------------------------------------------------------------------
    // 2. Early out: skip pixels with insufficient local contrast
    // -----------------------------------------------------------------------
    float range = lumaMax - lumaMin;
    if (range < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD))
        return rgbM;

    // -----------------------------------------------------------------------
    // 3. Subpixel aliasing correction
    //    Blend center toward 3x3 box average to remove subpixel shimmer.
    // -----------------------------------------------------------------------
    float3 rgbN = FxaaSample(uv + float2( 0, -1) * rcpFrame);
    float3 rgbS = FxaaSample(uv + float2( 0,  1) * rcpFrame);
    float3 rgbE = FxaaSample(uv + float2( 1,  0) * rcpFrame);
    float3 rgbW = FxaaSample(uv + float2(-1,  0) * rcpFrame);

    float lumaN = FxaaLuma(rgbN);
    float lumaS = FxaaLuma(rgbS);
    float lumaE = FxaaLuma(rgbE);
    float lumaW = FxaaLuma(rgbW);

    float lumaL = (lumaN + lumaS + lumaE + lumaW) * 0.25;
    float rangeL = abs(lumaL - lumaM);
    float blendL = saturate((rangeL / range - FXAA_SUBPIX_TRIM) * (1.0 / (1.0 - FXAA_SUBPIX_TRIM)));
    blendL = min(FXAA_SUBPIX_CAP, blendL) * FXAA_SUBPIX;

    float3 rgbL = (rgbNW + rgbN + rgbNE + rgbW + rgbM + rgbE + rgbSW + rgbS + rgbSE) * (1.0 / 9.0);

    // -----------------------------------------------------------------------
    // 4. Determine edge orientation
    //    Use a 3x3 Sobel-style gradient to separate horizontal from vertical.
    // -----------------------------------------------------------------------
    float edgeH =
        abs(lumaNW - lumaM) * 2.0 + abs(lumaN - lumaM) * 4.0 + abs(lumaNE - lumaM) * 2.0 +
        abs(lumaSW - lumaM) * 2.0 + abs(lumaS - lumaM) * 4.0 + abs(lumaSE - lumaM) * 2.0;
    float edgeV =
        abs(lumaNW - lumaM) * 2.0 + abs(lumaW - lumaM) * 4.0 + abs(lumaSW - lumaM) * 2.0 +
        abs(lumaNE - lumaM) * 2.0 + abs(lumaE - lumaM) * 4.0 + abs(lumaSE - lumaM) * 2.0;

    bool horzSpan = edgeH >= edgeV;

    // -----------------------------------------------------------------------
    // 5. Select the gradient pair perpendicular to the edge,
    //    then step half a pixel toward the dominant side.
    // -----------------------------------------------------------------------
    float luma1 = horzSpan ? lumaN : lumaW;
    float luma2 = horzSpan ? lumaS : lumaE;
    float grad1 = abs(luma1 - lumaM);
    float grad2 = abs(luma2 - lumaM);

    float lengthSign = horzSpan ? -rcpFrame.y : -rcpFrame.x;
    if (grad1 < grad2) lengthSign = -lengthSign;

    float2 posB = uv;
    if (horzSpan) posB.y += lengthSign * 0.5;
    else          posB.x += lengthSign * 0.5;

    // -----------------------------------------------------------------------
    // 6. Search along the edge in both directions for endpoints.
    //    An endpoint is where luma gradient falls below 1/4 of the peak.
    // -----------------------------------------------------------------------
    float2 offNP = horzSpan ? float2(rcpFrame.x, 0) : float2(0, rcpFrame.y);

    float lumaEdge  = lumaM + (grad1 < grad2 ? luma2 : luma1) * 0.5;
    float gradScale = max(grad1, grad2) * 0.25;

    float2 posN = posB - offNP;
    float2 posP = posB + offNP;
    bool doneN = false;
    bool doneP = false;
    float lumaEndN = 0;
    float lumaEndP = 0;

    [unroll(12)]
    for (int i = 0; i < 12; i++) {
        float step = SEARCH_STEPS[i];
        if (!doneN) {
            lumaEndN = FxaaLuma(FxaaSample(posN));
            doneN = abs(lumaEndN - lumaEdge) >= gradScale;
            if (!doneN) posN -= offNP * step;
        }
        if (!doneP) {
            lumaEndP = FxaaLuma(FxaaSample(posP));
            doneP = abs(lumaEndP - lumaEdge) >= gradScale;
            if (!doneP) posP += offNP * step;
        }
        if (doneN && doneP) break;
    }

    // -----------------------------------------------------------------------
    // 7. Compute the blend factor from position within the edge span.
    //    If the center is farther from the near endpoint, blend less.
    // -----------------------------------------------------------------------
    float dstN = horzSpan ? (uv.x - posN.x) : (uv.y - posN.y);
    float dstP = horzSpan ? (posP.x - uv.x) : (posP.y - uv.y);

    bool lumaMLtZero  = lumaM - lumaEdge < 0;
    bool spanLengthOk = (dstN < dstP)
        ? (lumaEndN - lumaEdge < 0) != lumaMLtZero
        : (lumaEndP - lumaEdge < 0) != lumaMLtZero;

    float spanLength = dstN + dstP;
    float dst = min(dstN, dstP);
    float pixelOffset = (spanLength > 0.0 && spanLengthOk)
        ? (0.5 - dst / spanLength)
        : 0.0;

    // -----------------------------------------------------------------------
    // 8. Final sample: combine edge offset with subpixel blend
    // -----------------------------------------------------------------------
    float pixelOffsetFinal = max(pixelOffset, blendL);

    float2 finalUV = uv;
    if (horzSpan) finalUV.y += pixelOffsetFinal * lengthSign;
    else          finalUV.x += pixelOffsetFinal * lengthSign;

    return lerp(FxaaSample(finalUV), rgbL, blendL);
}

// ---------------------------------------------------------------------------

PixelOutput PS(FullscreenVSOutput input) {
    uint w, h;
    ColorTexture.GetDimensions(w, h);
    float2 rcpFrame = 1.0 / float2(w, h);

    PixelOutput o;
    o.AAOutput = Fxaa(input.UV, rcpFrame);
    return o;
}
