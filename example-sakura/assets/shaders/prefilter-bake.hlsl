/*

@be-material: prefilter-bake-material {
    FaceIndex: float = 0
    Roughness: float = 0
    MaxSampleRadiance: float = 100
    EnvCubemap: textureCube = black-cube
    EnvSampler: sampler = linear-clamp
}

@be-shader prefilter-bake {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 main prefilter-bake-material

    target s0 PrefilteredFace float4
}

*/

/*========================================================*/
// region @be-auto-boilerplate
struct prefilter_bake_material {
    float FaceIndex;
    float Roughness;
    float MaxSampleRadiance;
};

cbuffer CBuffer_0 : register(b0, space0) {
    prefilter_bake_material _Main;
};
SamplerState EnvSampler : register(s1, space0);
TextureCube EnvCubemap : register(t2, space0);

struct PixelOutput {
    float4 PrefilteredFace : SV_Target0;
};

// endregion
/*========================================================*/

#include "ibl-common.hlsli"
#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 N = DirectionForFace((int)_Main.FaceIndex, input.UV);
    float3 R = N;
    float3 V = N;

    float roughness = _Main.Roughness;

    const uint SampleCount = 1024u;
    float3 prefiltered = float3(0.0, 0.0, 0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SampleCount; ++i) {
        float2 xi = Hammersley(i, SampleCount);
        float3 H = ImportanceSampleGGX(xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            float3 radiance = EnvCubemap.SampleLevel(EnvSampler, L, 0).rgb;

            if (_Main.MaxSampleRadiance > 0.0) {
                float maxComp = max(radiance.r, max(radiance.g, radiance.b));
                if (maxComp > _Main.MaxSampleRadiance) {
                    radiance *= _Main.MaxSampleRadiance / maxComp;
                }
            }

            prefiltered += radiance * NdotL;
            totalWeight += NdotL;
        }
    }

    prefiltered /= max(totalWeight, 0.0001);

    PixelOutput output;
    output.PrefilteredFace = float4(prefiltered, 1.0);
    return output;
}
