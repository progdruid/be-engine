
/*

@be-material: standard-phong-material {
    DiffuseColor: float3 = (1.0, 1.0, 1.0)
    SpecularColor: float3 = (1.0, 1.0, 1.0)
    Shininess: float = 0.0
    EmissiveColor: float3 = (1.0, 1.0, 1.0)
    DiffuseTexture: texture2d = white
    SpecularTexture: texture2d = black
    EmissiveTexture: texture2d = black
    NormalMap: texture2d = flat-normal
    InputSampler: sampler = anisotropic-wrap
}

@be-shader standard-phong {
    topology triangle-list
    rasterizer back-solid
    blend disable
    depth less

    vertex VertexFunction(position, normal, uv0, tangent)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main standard-phong-material

    target s0 Albedo_RGB float3
    target s1 WorldNormal_XYZ float4
    target s2 SurfaceData float4
    target s3 EmissiveRGB float3
}

*/


/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"
#include "objectMaterial.hlsl"

struct standard_phong_material {
    float3 DiffuseColor;
    float3 SpecularColor;
    float Shininess;
    float3 EmissiveColor;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    standard_phong_material _GeometryMain;
};
SamplerState InputSampler : register(s1, space2);
Texture2D DiffuseTexture : register(t2, space2);
Texture2D SpecularTexture : register(t3, space2);
Texture2D EmissiveTexture : register(t4, space2);
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
    float4 SurfaceData : SV_Target2;
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
    float4 diffuse  = DiffuseTexture.Sample(InputSampler, input.UV);
    if (diffuse.a < 0.5) discard;
    float3 specular  = SpecularTexture.Sample(InputSampler, input.UV).rgb;
    float3 emissive  = EmissiveTexture.Sample(InputSampler, input.UV).rgb;

    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent.xyz);
    float3 B = cross(N, T) * input.Tangent.w;
    float3x3 TBN = float3x3(T, B, N);
    float3 normalSample = NormalMap.Sample(InputSampler, input.UV).rgb * 2.0 - 1.0;
    float3 worldNormal  = normalize(mul(normalSample, TBN));

    PixelOutput output;
    output.Albedo_RGB          = diffuse.rgb * _GeometryMain.DiffuseColor;
    output.WorldNormal_XYZ.xyz = worldNormal;
    output.WorldNormal_XYZ.w   = 1.0;  // flag: Blinn-Phong pipeline
    output.SurfaceData.rgb     = specular * _GeometryMain.SpecularColor;
    output.SurfaceData.a       = _GeometryMain.Shininess;  // 0-1 normalized, lighting shader multiplies by 2048
    output.EmissiveRGB         = emissive * _GeometryMain.EmissiveColor;
    return output;
}
