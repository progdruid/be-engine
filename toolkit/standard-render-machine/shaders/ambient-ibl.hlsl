/*

@be-material: ambient-ibl-material {
    MaxMipLevel: float = 5
    Albedo_RGB: texture2d = black
    WorldNormal_XYZ: texture2d = black
    ORM_RGB: texture2d = black
    Depth_Tex: texture2d = black
    IrradianceCubemap: textureCube = black-cube
    PrefilteredCubemap: textureCube = black-cube
    BrdfLut: texture2d = black
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
#include "core/be-bindless-tables.hlsl"
#include "core/uniform-material.hlsl"

struct ambient_ibl_material {
    float MaxMipLevel;
};

struct DrawRoot {
    uniform_material* Frame;
    ambient_ibl_material* Main;
    uint Albedo_RGB;
    uint WorldNormal_XYZ;
    uint ORM_RGB;
    uint Depth_Tex;
    uint IrradianceCubemap;
    uint PrefilteredCubemap;
    uint BrdfLut;
    uint InputSampler;
};
[[vk::push_constant]] DrawRoot Root;

property uniform_material* _Frame { get { return Root.Frame; } }
property ambient_ibl_material* _Main { get { return Root.Main; } }
property Texture2D Albedo_RGB { get { return Tex2DTable[Root.Albedo_RGB]; } }
property Texture2D WorldNormal_XYZ { get { return Tex2DTable[Root.WorldNormal_XYZ]; } }
property Texture2D ORM_RGB { get { return Tex2DTable[Root.ORM_RGB]; } }
property Texture2D Depth_Tex { get { return Tex2DTable[Root.Depth_Tex]; } }
property TextureCube IrradianceCubemap { get { return TexCubeTable[Root.IrradianceCubemap]; } }
property TextureCube PrefilteredCubemap { get { return TexCubeTable[Root.PrefilteredCubemap]; } }
property Texture2D BrdfLut { get { return Tex2DTable[Root.BrdfLut]; } }
property SamplerState InputSampler { get { return SamplerTable[Root.InputSampler]; } }

struct PixelOutput {
    float3 AmbientHDR : SV_Target0;
};

// endregion
/*========================================================*/

#include "core/fullscreen-vertex.hlsl"
#include "core/BeFunctions.hlsli"

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max((1.0).xxx - roughness, F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 rawNormal = WorldNormal_XYZ.Sample(InputSampler, input.UV).xyz;
    float normalLength = length(rawNormal);

    PixelOutput output;
    if (normalLength < 0.001) {
        output.AmbientHDR = _Frame.AmbientColor;
        return output;
    }

    float3 N = rawNormal / normalLength;
    float3 albedo = Albedo_RGB.Sample(InputSampler, input.UV).rgb;
    float3 orm = ORM_RGB.Sample(InputSampler, input.UV).rgb;
    float ao = orm.r;
    float roughness = orm.g;
    float metallic = orm.b;

    float3 irradiance = IrradianceCubemap.Sample(InputSampler, N).rgb;
    float3 kD = (1.0 - metallic) * (1.0).xxx;
    float3 diffuse = kD * albedo * irradiance;

    float depth = Depth_Tex.Sample(InputSampler, input.UV).r;
    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _Frame.CameraInverseProjectionView);
    float3 V = normalize(_Frame.CameraPosition - worldPos);
    float3 R = reflect(-V, N);

    float NdotV = max(dot(N, V), 0.0);
    float3 F0 = lerp((0.04).xxx, albedo, metallic);
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

    float3 prefilteredRadiance = PrefilteredCubemap.SampleLevel(InputSampler, R, roughness * _Main.MaxMipLevel).rgb;
    float2 brdf = BrdfLut.Sample(InputSampler, float2(NdotV, roughness)).rg;

    float3 specular = prefilteredRadiance * (F * brdf.x + brdf.y);

    float3 ambient = (diffuse + specular) * ao;
    ambient += albedo * _Frame.AmbientColor;
    output.AmbientHDR = ambient;
    return output;
}
