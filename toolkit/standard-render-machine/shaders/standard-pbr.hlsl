
/*

@be-material: standard-pbr-material {
    BaseColor: float3 = (1.0, 1.0, 1.0)
    Metallic: float = 0.0
    Roughness: float = 0.5
    AO: float = 1.0
    EmissiveColor: float3 = (0.0, 0.0, 0.0)
    Diffuse_or_Albedo: texture2d = white
    ORM_RGB: texture2d = default-orm
    Emissive_RGB: texture2d = white
    NormalMap: texture2d = flat-normal
    InputSampler: sampler = anisotropic-wrap
}

@be-shader standard-pbr {
    topology triangle-list
    rasterizer back-solid
    blend disable
    depth less

    vertex VertexFunction(position, normal, uv0, tangent)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main standard-pbr-material

    target s0 Albedo_RGB float3
    target s1 WorldNormal_XYZ float4
    target s2 ORM_RGB float4
    target s3 EmissiveRGB float3
}

*/


/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"
#include "objectMaterial.hlsl"

struct standard_pbr_material {
    float3 BaseColor;
    float Metallic;
    float Roughness;
    float AO;
    float3 EmissiveColor;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    standard_pbr_material _GeometryMain;
};
SamplerState InputSampler : register(s1, space2);
Texture2D Diffuse_or_Albedo : register(t2, space2);
Texture2D ORM_RGB : register(t3, space2);
Texture2D Emissive_RGB : register(t4, space2);
Texture2D NormalMap : register(t5, space2);

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float4 Tangent : TANGENT;
};

struct PixelOutput {
    float3 Albedo_RGB : SV_Target0;
    float4 WorldNormal_XYZ : SV_Target1;
    float4 ORM_RGB : SV_Target2;
    float3 EmissiveRGB : SV_Target3;
};

// endregion
/*========================================================*/


struct Interpolators {
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float4 Tangent  : TANGENT;   // xyz=tangent, w=handedness
    float2 UV       : TEXCOORD0;
};

Interpolators VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _GeometryObject.Model);
    float3x3 normalMatrix = (float3x3)_GeometryObject.Model;

    Interpolators output;
    output.Position       = mul(worldPosition, _GeometryObject.ProjectionView);
    output.Normal         = normalize(mul(input.Normal, normalMatrix));
    output.Tangent        = float4(normalize(mul(input.Tangent.xyz, normalMatrix)), input.Tangent.w);
    output.UV             = input.UV;

    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float4 diffuse_albedo = Diffuse_or_Albedo.Sample(InputSampler, input.UV);
    if (diffuse_albedo.a < 0.5) discard;
    float4 orm      = ORM_RGB.Sample(InputSampler, input.UV);
    float3 emissive = Emissive_RGB.Sample(InputSampler, input.UV).rgb;

    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent.xyz);
    float3 B = cross(N, T) * input.Tangent.w;
    float3x3 TBN = float3x3(T, B, N);
    float3 normalSample = NormalMap.Sample(InputSampler, input.UV).rgb * 2.0 - 1.0;
    // Toksvig factor: the mipped (non-renormalized) normal shrinks where sub-pixel normal variance is high
    float toksvig = saturate(length(normalSample));
    float3 worldNormal  = normalize(mul(normalSample, TBN));

    // roughen the surface to match the footprint's normal spread, killing specular aliasing.
    // routed through the Phong-exponent relation (s = 2/a^2 - 2) so the same Toksvig factor drives both pipelines
    float roughness = clamp(orm.g * _GeometryMain.Roughness, 0.04, 1.0);
    float specPower = 2.0 / (roughness * roughness) - 2.0;
    float toksvigPower = (toksvig < 1.0)
        ? (toksvig * specPower) / (toksvig + specPower * (1.0 - toksvig))
        : specPower;
    float toksvigRoughness = sqrt(2.0 / (toksvigPower + 2.0));

    // Geometric specular AA: estimate screen-space shading-normal variance and fold it in as extra roughness.
    // catches curvature/geometry aliasing that the texture-space Toksvig term can't see.
    float3 dNdx = ddx(worldNormal);
    float3 dNdy = ddy(worldNormal);
    float geoVariance = 0.5 * (dot(dNdx, dNdx) + dot(dNdy, dNdy));
    float kernelRoughness2 = min(2.0 * geoVariance, 0.18);
    float finalRoughness = sqrt(saturate(toksvigRoughness * toksvigRoughness + kernelRoughness2));

    PixelOutput output;
    output.Albedo_RGB           = diffuse_albedo.rgb * _GeometryMain.BaseColor;
    output.WorldNormal_XYZ.xyz  = worldNormal;
    output.WorldNormal_XYZ.w    = 0.0;
    output.ORM_RGB              = float4(
        orm.r * _GeometryMain.AO,
        finalRoughness,
        orm.b * _GeometryMain.Metallic,
        0.0
    );
    output.EmissiveRGB          = emissive * _GeometryMain.EmissiveColor;
    return output;
};
