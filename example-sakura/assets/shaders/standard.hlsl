
/*

@be-material: standard-material-for-geometry-pass
[
    "DiffuseColor: float3 = [1.0, 1.0, 1.0]",
    "SpecularColor: float3 = [1.0, 1.0, 1.0]",
    "Shininess: float = 0.0",
    "EmissiveColor: float3 = [0.0, 0.0, 0.0]",

    "DiffuseTexture: texture2d(0) = white",
    "SpecularTexture: texture2d(1) = black",
    "EmissiveTexture: texture2d(2) = white",

    "InputSampler: sampler(0) = linear-clamp",
    //"InputSampler: sampler(0) = point-clamp",
]
@be-end


@be-shader: standard
{
    "topology": "triangle-list",
    "vertex": "VertexFunction",
    "vertexLayout": ["position", "normal", "uv0"],
    "pixel": "PixelFunction",
    "materials": {
        "geometry-object": { "scheme": "object-material-for-geometry-pass", "slot": 1 },
        "geometry-main": { "scheme": "standard-material-for-geometry-pass", "slot": 2 },
    },
    "targets": {
        "DiffuseRGB":             { "type": "float3", "slot": 0 },
        "WorldNormalXYZ_UnusedA": { "type": "float4", "slot": 1 },
        "SpecularRGB_ShininessA": { "type": "float4", "slot": 2 },
        "EmissiveRGB":            { "type": "float3", "slot": 3 },
    }
}
@be-end

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "objectMaterial.hlsl"

struct standard_material_for_geometry_pass {
    float3 DiffuseColor;
    float3 SpecularColor;
    float Shininess;
    float3 EmissiveColor;
};

Texture2D DiffuseTexture : register(t0);
Texture2D SpecularTexture : register(t1);
Texture2D EmissiveTexture : register(t2);
SamplerState InputSampler : register(s0);

cbuffer CBuffer1 : register(b1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer2 : register(b2) {
    standard_material_for_geometry_pass _GeometryMain;
};

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
};

struct PixelOutput {
    float3 DiffuseRGB : SV_Target0;
    float4 WorldNormalXYZ_UnusedA : SV_Target1;
    float4 SpecularRGB_ShininessA : SV_Target2;
    float3 EmissiveRGB : SV_Target3;
};

// endregion
/*========================================================*/

struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV    : TEXCOORD0;
};

VertexOutput VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _GeometryObject.Model);
    
    VertexOutput output;
    output.Position = mul(worldPosition, _GeometryObject.ProjectionView);
    output.Normal = normalize(mul(input.Normal, (float3x3)_GeometryObject.Model));
    output.UV = input.UV;

    return output;
}

PixelOutput PixelFunction(VertexOutput input) {
    float4 diffuseColor = DiffuseTexture.Sample(InputSampler, input.UV);
    if (diffuseColor.a < 0.5) discard;
    float4 specularColor = SpecularTexture.Sample(InputSampler, input.UV);
    float3 emissiveColor = EmissiveTexture.Sample(InputSampler, input.UV).rgb;
    
    PixelOutput output;
    output.DiffuseRGB = diffuseColor.rgb * _GeometryMain.DiffuseColor;
    output.WorldNormalXYZ_UnusedA.xyz = normalize(input.Normal);
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = specularColor.rgb * _GeometryMain.SpecularColor;
    output.SpecularRGB_ShininessA.a = _GeometryMain.Shininess / 2048.0;
    output.EmissiveRGB = emissiveColor.rgb * _GeometryMain.EmissiveColor;
    
    return output;
};
