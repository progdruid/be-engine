/*

@be-material: rift-terrain-material {
    DiffuseColor: float3 = (0.5, 0.5, 0.5)
    SpecularColor: float3 = (-0.2, -0.2, -0.1)
    Shininess: float = 0.0
    HeightMap: texture2d = black
    MapSize: float = 2880.0
    MapResolution: float = 720.0
    HeightScale: float = 1.0
    HeightSampler: sampler = point-wrap
}

@be-shader rift-terrain {
    topology patch-list-3

    vertex VertexFunction(position)
    hull HullFunction
    domain DomainFunction
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main rift-terrain-material Terrain

    target s0 DiffuseRGB float3
    target s1 WorldNormalXYZ_UnusedA float4
    target s2 SpecularRGB_ShininessA float4
    target s3 EmissiveRGB float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/uniform-material.hlsl"
#include "core/objectMaterial.hlsl"

struct rift_terrain_material {
    float3 DiffuseColor;
    float3 SpecularColor;
    float Shininess;
    float MapSize;
    float MapResolution;
    float HeightScale;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    rift_terrain_material _Terrain;
};
SamplerState HeightSampler : register(s1, space2);
Texture2D HeightMap : register(t2, space2);

struct VertexInput {
    float3 Position : POSITION;
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
    nointerpolation float3 Normal : NORMAL;
    float3 WorldPosition : TEXCOORD1;
};

Interpolators VertexFunction(VertexInput input) {
    float4 worldFlat = mul(float4(input.Position, 1.0), _GeometryObject.Model);

    float2 hUV = float2(0.5 + worldFlat.x / _Terrain.MapSize,
                        0.5 + worldFlat.z / _Terrain.MapSize);
    float2 texUV = hUV + 0.5 / _Terrain.MapResolution;
    float height = HeightMap.SampleLevel(HeightSampler, texUV, 0).r * _Terrain.HeightScale;

    float3 worldPos = float3(worldFlat.x, worldFlat.y + height, worldFlat.z);

    Interpolators output;
    output.Position = mul(float4(worldPos, 1.0), _GeometryObject.ProjectionView);
    output.Normal = float3(0, 0, 0);
    output.WorldPosition = worldPos;
    return output;
}

struct PatchConstantOutput {
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};

PatchConstantOutput PatchConstantFunction(InputPatch<Interpolators, 3> patch) {
    PatchConstantOutput output;
    output.EdgeTessFactor[0] = 1.0f;
    output.EdgeTessFactor[1] = 1.0f;
    output.EdgeTessFactor[2] = 1.0f;
    output.InsideTessFactor = 1.0f;
    return output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunction")]
Interpolators HullFunction(InputPatch<Interpolators, 3> patch, uint pointId : SV_OutputControlPointID) {
    Interpolators output = patch[pointId];

    float3 v0 = patch[0].WorldPosition;
    float3 v1 = patch[1].WorldPosition;
    float3 v2 = patch[2].WorldPosition;
    output.Normal = normalize(cross(v1 - v0, v2 - v0));

    return output;
}

[domain("tri")]
Interpolators DomainFunction(PatchConstantOutput patchData, float3 barycentric : SV_DomainLocation, const OutputPatch<Interpolators, 3> patch) {
    Interpolators output;
    output.WorldPosition = barycentric.x * patch[0].WorldPosition + barycentric.y * patch[1].WorldPosition + barycentric.z * patch[2].WorldPosition;
    output.Position = barycentric.x * patch[0].Position + barycentric.y * patch[1].Position + barycentric.z * patch[2].Position;
    output.Normal = barycentric.x * patch[0].Normal + barycentric.y * patch[1].Normal + barycentric.z * patch[2].Normal;
    output.Normal = normalize(output.Normal);
    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    PixelOutput output;
    output.DiffuseRGB = _Terrain.DiffuseColor;
    output.WorldNormalXYZ_UnusedA.xyz = normalize(input.Normal);
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = _Terrain.SpecularColor;
    output.SpecularRGB_ShininessA.a = _Terrain.Shininess / 2048.0;
    output.EmissiveRGB = float3(0, 0, 0);
    return output;
}
