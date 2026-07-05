/*

@be-material: irradiance-bake-material {
    FaceIndex: float = 0
    MaxSampleRadiance: float = 100
    EnvCubemap: textureCube = black-cube
    EnvSampler: sampler = linear-clamp
}

@be-shader irradiance-bake {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 main irradiance-bake-material

    target s0 IrradianceFace float4
}

*/

/*========================================================*/
// region @be-auto-boilerplate
struct irradiance_bake_material {
    float FaceIndex;
    float MaxSampleRadiance;
};

cbuffer CBuffer_0 : register(b0, space0) {
    irradiance_bake_material _Main;
};
SamplerState EnvSampler : register(s1, space0);
TextureCube EnvCubemap : register(t2, space0);

struct PixelOutput {
    float4 IrradianceFace : SV_Target0;
};

// endregion
/*========================================================*/

#include "ibl-common.hlsli"
#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 N = DirectionForFace((int)_Main.FaceIndex, input.UV);

    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float3 irradiance = float3(0.0, 0.0, 0.0);
    float sampleDelta = 0.025;
    float sampleCount = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
            float3 radiance = EnvCubemap.SampleLevel(EnvSampler, sampleVec, 0).rgb;

            if (_Main.MaxSampleRadiance > 0.0) {
                float maxComp = max(radiance.r, max(radiance.g, radiance.b));
                if (maxComp > _Main.MaxSampleRadiance) {
                    radiance *= _Main.MaxSampleRadiance / maxComp;
                }
            }

            irradiance += radiance * cos(theta) * sin(theta);
            sampleCount += 1.0;
        }
    }

    irradiance = PI * irradiance / sampleCount;

    PixelOutput output;
    output.IrradianceFace = float4(irradiance, 1.0);
    return output;
}
