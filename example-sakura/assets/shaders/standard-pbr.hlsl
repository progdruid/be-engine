
/*

@be-material: standard-pbr-material
[
    "BaseColor: float3 = [1.0, 1.0, 1.0]",

    "Metallic: float = 0.0",
    "Roughness: float = 0.5",
    "AO: float = 1.0",

    "EmissiveColor: float3 = [0.0, 0.0, 0.0]",

    "Diffuse_or_Albedo: texture2d = white",
    "ORM_RGB: texture2d = default-orm",
    "Emissive_RGB: texture2d = white",

    "InputSampler: sampler = linear-clamp",
]
@be-end

@be-shader: standard-pbr
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
        "geometry-main": { "scheme": "standard-pbr-material", "slot": 2 },
    },
    
    "targets": {
        "Albedo_RGB":      { "type": "float3", "slot": 0 },
        "WorldNormal_XYZ": { "type": "float4", "slot": 1 },
        "ORM_RGB":         { "type": "float4", "slot": 2 },
        "EmissiveRGB":     { "type": "float3", "slot": 3 },
    }
}
@be-end
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

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
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
    float4 diffuse_albedo = Diffuse_or_Albedo.Sample(InputSampler, input.UV);
    if (diffuse_albedo.a < 0.5) discard;
    float4 orm      = ORM_RGB.Sample(InputSampler, input.UV);
    float3 emissive = Emissive_RGB.Sample(InputSampler, input.UV).rgb;

    PixelOutput output;
    output.Albedo_RGB           = diffuse_albedo.rgb * _GeometryMain.BaseColor;
    output.WorldNormal_XYZ.xyz  = normalize(input.Normal);
    output.WorldNormal_XYZ.w    = 0.0;
    output.ORM_RGB              = float4(
        orm.r * _GeometryMain.AO,
        orm.g * _GeometryMain.Roughness,
        orm.b * _GeometryMain.Metallic,
        0.0
    );
    output.EmissiveRGB          = emissive * _GeometryMain.EmissiveColor;
    return output;
};
