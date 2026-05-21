
/*

@be-material: standard-phong-material
[
    "DiffuseColor: float3 = [1.0, 1.0, 1.0]",
    "SpecularColor: float3 = [1.0, 1.0, 1.0]",
    "Shininess: float = 0.0",
    "EmissiveColor: float3 = [1.0, 1.0, 1.0]",

    "DiffuseTexture: texture2d = white",
    "SpecularTexture: texture2d = black",
    "EmissiveTexture: texture2d = black",

    "InputSampler: sampler = linear-clamp",
]
@be-end

@be-shader: standard-phong
{
    "topology": "triangle-list",
    "rasterizer": "back-solid",
    "blend": "disable",
    "depthStencil": "less",

    "vertex": "VertexFunction",
    "vertexLayout": ["position", "normal", "uv0"],
    "pixel": "PixelFunction",

    "materials": {
        "frame": { "scheme": "uniform-material", "slot": 0 },
        "geometry-object": { "scheme": "object-material-for-geometry-pass", "slot": 1 },
        "geometry-main": { "scheme": "standard-phong-material", "slot": 2 },
    },

    "targets": {
        "Albedo_RGB":      { "type": "float3", "slot": 0 },
        "WorldNormal_XYZ": { "type": "float4", "slot": 1 },
        "SurfaceData":     { "type": "float4", "slot": 2 },
        "EmissiveRGB":     { "type": "float3", "slot": 3 }
    }
}
@be-end

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

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
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
    float3 Normal : NORMAL;
    float2 UV    : TEXCOORD0;
};

Interpolators VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _GeometryObject.Model);

    Interpolators output;
    output.Position = mul(worldPosition, _GeometryObject.ProjectionView);
    output.Normal = normalize(mul(input.Normal, (float3x3)_GeometryObject.Model));
    output.UV = input.UV;

    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float4 diffuse  = DiffuseTexture.Sample(InputSampler, input.UV);
    if (diffuse.a < 0.5) discard;
    float3 specular  = SpecularTexture.Sample(InputSampler, input.UV).rgb;
    float3 emissive  = EmissiveTexture.Sample(InputSampler, input.UV).rgb;

    PixelOutput output;
    output.Albedo_RGB          = diffuse.rgb * _GeometryMain.DiffuseColor;
    output.WorldNormal_XYZ.xyz = normalize(input.Normal);
    output.WorldNormal_XYZ.w   = 1.0;  // flag: Blinn-Phong pipeline
    output.SurfaceData.rgb     = specular * _GeometryMain.SpecularColor;
    output.SurfaceData.a       = _GeometryMain.Shininess;  // 0-1 normalized, lighting shader multiplies by 2048
    output.EmissiveRGB         = emissive * _GeometryMain.EmissiveColor;
    return output;
}
