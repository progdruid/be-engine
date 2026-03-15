
/*

@be-material: checkerboard-material-for-geometry-pass
[
    "DiffuseColor: float3 = [1.0, 1.0, 1.0]",
    "SpecularColor: float3 = [0.3, 0.3, 0.3]",
    "Shininess: float = 0.0",
    "TileScale: float = 1.0",

    "DiffuseTexture: texture2d = black",
    "SpecularTexture: texture2d = black",

    "InputSampler: sampler = point-wrap",
]
@be-end


@be-shader: checkerboard
{
    "topology": "triangle-list",
    "vertex": "VertexFunction",
    "vertexLayout": ["position", "normal" ],
    "pixel": "PixelFunction",
    "materials": {
        "geometry-object": { "scheme": "object-material-for-geometry-pass", "slot": 1 },
        "geometry-main": { "scheme": "checkerboard-material-for-geometry-pass", "slot": 2 },
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

struct checkerboard_material_for_geometry_pass {
    float3 DiffuseColor;
    float3 SpecularColor;
    float Shininess;
    float TileScale;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    checkerboard_material_for_geometry_pass _GeometryMain;
};
SamplerState InputSampler : register(s1, space2);
Texture2D DiffuseTexture : register(t2, space2);
Texture2D SpecularTexture : register(t3, space2);

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct PixelOutput {
    float3 DiffuseRGB : SV_Target0;
    float4 WorldNormalXYZ_UnusedA : SV_Target1;
    float4 SpecularRGB_ShininessA : SV_Target2;
    float3 EmissiveRGB : SV_Target3;
};

// endregion
/*========================================================*/

struct Interpolators {
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
    float3 Normal : NORMAL;
};

Interpolators VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _GeometryObject.Model);

    Interpolators output;
    output.Position = mul(worldPosition, _GeometryObject.ProjectionView);
    output.WorldPosition = worldPosition.xyz;
    output.Normal = normalize(mul(input.Normal, (float3x3)_GeometryObject.Model));

    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    input.Normal = normalize(input.Normal);
    
    // Triplanar mapping using world coordinates
    // This ensures texture stays consistent regardless of object scale/transform
    float3 worldPos = input.WorldPosition * _GeometryMain.TileScale;
    float3 blendWeights = abs(input.Normal);

    // Normalise blend weights so they sum to 1
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

    // Sample texture from each axis projection
    float4 xDiffuse  =  DiffuseTexture.Sample(InputSampler, worldPos.yz);
    float4 xSpecular = SpecularTexture.Sample(InputSampler, worldPos.yz);
    float4 yDiffuse  =  DiffuseTexture.Sample(InputSampler, worldPos.xz);
    float4 ySpecular = SpecularTexture.Sample(InputSampler, worldPos.xz);
    float4 zDiffuse  =  DiffuseTexture.Sample(InputSampler, worldPos.xy);
    float4 zSpecular = SpecularTexture.Sample(InputSampler, worldPos.xy);

    // Blend based on surface normal
    float4 triplanarDiffuse = 
        xDiffuse * blendWeights.x +
        yDiffuse * blendWeights.y +
        zDiffuse * blendWeights.z;
    
    float4 triplanarSpecular =
        xSpecular * blendWeights.x +
        ySpecular * blendWeights.y +
        zSpecular * blendWeights.z;
    
    PixelOutput output;
    output.DiffuseRGB = triplanarDiffuse.rgb * _GeometryMain.DiffuseColor;
    output.WorldNormalXYZ_UnusedA.xyz = input.Normal;
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = triplanarSpecular.xyz * _GeometryMain.SpecularColor;
    output.SpecularRGB_ShininessA.a = _GeometryMain.Shininess / 2048.0;
    output.EmissiveRGB = (0.f).rrr;

    return output;
};
