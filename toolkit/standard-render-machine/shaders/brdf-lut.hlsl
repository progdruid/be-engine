/*

@be-material: brdf-lut-material {
}

@be-shader brdf-lut {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 main brdf-lut-material

    target s0 BrdfLut float2
}

*/

/*========================================================*/
// region @be-auto-boilerplate
struct PixelOutput {
    float2 BrdfLut : SV_Target0;
};

// endregion
/*========================================================*/

#include "ibl-common.hlsli"
#include "core/fullscreen-vertex.hlsl"

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float2 IntegrateBRDF(float NdotV, float roughness) {
    float3 V = float3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    float3 N = float3(0.0, 0.0, 1.0);

    float2 integral = float2(0.0, 0.0);
    const uint SampleCount = 1024u;

    for (uint i = 0u; i < SampleCount; ++i) {
        float2 xi = Hammersley(i, SampleCount);
        float3 H = ImportanceSampleGGX(xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0) {
            float G = GeometrySmith(NdotV, NdotL, roughness);
            float Fc = pow(1.0 - VdotH, 5.0);
            integral.x += (1.0 - Fc) * G * VdotH / (NdotH * NdotV);
            integral.y += Fc * G * VdotH / (NdotH * NdotV);
        }
    }

    integral /= float(SampleCount);
    return integral;
}

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float NdotV = input.UV.x;
    float roughness = input.UV.y;

    float2 brdf = IntegrateBRDF(NdotV, roughness);

    PixelOutput output;
    output.BrdfLut = brdf;
    return output;
}
