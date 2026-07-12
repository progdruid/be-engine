
/*

@be-material: checkerboard-material-for-geometry-pass {
    DiffuseColor: float3 = (1.0, 1.0, 1.0)
    Metallic: float = 0.0
    Roughness: float = 0.5
    AO: float = 1.0
    TileScale: float = 1.0
    DiffuseTexture: texture2d = black
    InputSampler: sampler = point-wrap
}


@be-shader checkerboard {
    topology triangle-list

    vertex VertexFunction(position, normal)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main checkerboard-material-for-geometry-pass

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

struct checkerboard_material_for_geometry_pass {
    float3 DiffuseColor;
    float Metallic;
    float Roughness;
    float AO;
    float TileScale;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    checkerboard_material_for_geometry_pass _GeometryMain;
};
SamplerState InputSampler : register(s1, space2);
Texture2D DiffuseTexture : register(t2, space2);

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
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

    float3 worldPos = input.WorldPosition * _GeometryMain.TileScale;
    float3 blendWeights = abs(input.Normal);
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

    float4 xDiffuse = DiffuseTexture.Sample(InputSampler, worldPos.yz);
    float4 yDiffuse = DiffuseTexture.Sample(InputSampler, worldPos.xz);
    float4 zDiffuse = DiffuseTexture.Sample(InputSampler, worldPos.xy);

    float4 triplanarDiffuse =
        xDiffuse * blendWeights.x +
        yDiffuse * blendWeights.y +
        zDiffuse * blendWeights.z;

    PixelOutput output;
    output.Albedo_RGB          = triplanarDiffuse.rgb * _GeometryMain.DiffuseColor;
    output.WorldNormal_XYZ.xyz = input.Normal;
    output.WorldNormal_XYZ.w   = 0.0;
    output.ORM_RGB             = float4(_GeometryMain.AO, _GeometryMain.Roughness, _GeometryMain.Metallic, 0.0);
    output.EmissiveRGB         = (0.f).rrr;
    return output;
};
