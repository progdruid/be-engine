/*

@be-material: dof-material {
    FocalDistance: float = 10.0
    FocalRange: float = 8.0
    MaxBokehRadius: float = 12.0
    ColorInput: texture2d = black
    DepthInput: texture2d = white
    LinearSampler: sampler = linear-clamp
    PointSampler: sampler = point-clamp
}

@be-shader dof {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PS

    bind s0 frame uniform-material
    bind s1 main dof-material

    target s0 DofOutput float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct dof_material {
    float FocalDistance;
    float FocalRange;
    float MaxBokehRadius;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    dof_material _Main;
};
SamplerState LinearSampler : register(s1, space1);
SamplerState PointSampler  : register(s2, space1);
Texture2D ColorInput       : register(t3, space1);
Texture2D DepthInput       : register(t4, space1);

struct PixelOutput {
    float3 DofOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

#define NUM_SAMPLES  48
#define GOLDEN_ANGLE 2.39996322972865332

float LinearDepth(float d) {
    float near = _Frame.NearFarPlane.x;
    float far  = _Frame.NearFarPlane.y;
    return (near * far) / (far - d * (far - near));
}

// Returns signed CoC in pixels.
// Positive = behind focus (far field), negative = in front (near field).
float ComputeCoC(float linearDepth) {
    float dist = linearDepth - _Main.FocalDistance;
    return clamp(dist / max(_Main.FocalRange, 0.001), -1.0, 1.0) * _Main.MaxBokehRadius;
}

PixelOutput PS(FullscreenVSOutput input) {
    uint w, h;
    ColorInput.GetDimensions(w, h);
    float2 texelSize = 1.0 / float2(w, h);

    float rawDepth  = DepthInput.SampleLevel(PointSampler, input.UV, 0).r;
    float depth     = LinearDepth(rawDepth);
    float coc       = ComputeCoC(depth);
    float radius    = abs(coc);

    float3 sharp = ColorInput.SampleLevel(LinearSampler, input.UV, 0).rgb;

    if (radius < 0.5) {
        PixelOutput o;
        o.DofOutput = sharp;
        return o;
    }

    float3 accum       = float3(0.0, 0.0, 0.0);
    float  totalWeight = 0.0;

    [loop]
    for (int i = 0; i < NUM_SAMPLES; i++) {
        float  theta     = float(i) * GOLDEN_ANGLE;
        float  r         = sqrt((float(i) + 0.5) / float(NUM_SAMPLES));
        float2 sampleUV  = input.UV + float2(cos(theta), sin(theta)) * r * radius * texelSize;

        float sampleRaw  = DepthInput.SampleLevel(PointSampler, sampleUV, 0).r;
        float sampleCoC  = abs(ComputeCoC(LinearDepth(sampleRaw)));

        // Blurry neighbours scatter more; prevents sharp pixels bleeding into
        // an out-of-focus region.
        float weight = max(sampleCoC / max(radius, 0.001), 0.01);

        accum       += ColorInput.SampleLevel(LinearSampler, sampleUV, 0).rgb * weight;
        totalWeight += weight;
    }

    float3 blurred = accum / totalWeight;

    // Smooth ramp: fully sharp at radius 0.5, fully blurred at radius 2.5+
    float blend = saturate((radius - 0.5) / 2.0);

    PixelOutput o;
    o.DofOutput = lerp(sharp, blurred, blend);
    return o;
}
