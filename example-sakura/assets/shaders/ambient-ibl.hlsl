/*

@be-material: ambient-ibl-material {
    Albedo_RGB: texture2d = black
    WorldNormal_XYZ: texture2d = black
    ORM_RGB: texture2d = black
    IrradianceCubemap: textureCube = black-cube
    InputSampler: sampler = linear-clamp
}

@be-shader ambient-ibl {
    topology triangle-strip
    rasterizer back-solid
    blend additive
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main ambient-ibl-material

    target s0 AmbientHDR float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

SamplerState InputSampler : register(s1, space1);
Texture2D Albedo_RGB : register(t2, space1);
Texture2D WorldNormal_XYZ : register(t3, space1);
Texture2D ORM_RGB : register(t4, space1);
TextureCube IrradianceCubemap : register(t5, space1);

struct PixelOutput {
    float3 AmbientHDR : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 rawNormal = WorldNormal_XYZ.Sample(InputSampler, input.UV).xyz;
    float normalLength = length(rawNormal);

    PixelOutput output;
    if (normalLength < 0.001) {
        output.AmbientHDR = float3(0.0, 0.0, 0.0);
        return output;
    }

    float3 N = rawNormal / normalLength;
    float3 albedo = Albedo_RGB.Sample(InputSampler, input.UV).rgb;
    float metallic = ORM_RGB.Sample(InputSampler, input.UV).b;

    float3 irradiance = IrradianceCubemap.Sample(InputSampler, N).rgb;
    float3 diffuse = albedo * irradiance * (1.0 - metallic);

    output.AmbientHDR = diffuse;
    return output;
}
