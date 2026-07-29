/*

@be-material: terrain-main-material-for-geometry-pass {
    DiffuseColor: float3 = (0.5, 0.5, 0.5)
    SpecularColor: float3 = (-0.2, -0.2, -0.1)
    Shininess: float = 0.0
    DiffuseTexture: texture2d = white
    TerrainScale: float = 1.0
    HeightScale: float = 1.0
    NoiseResolution: float = 4.0
    Speed: float = 0.2
    InputSampler: sampler = point-clamp
}

@be-shader terrain {
    topology patch-list-3

    vertex VertexFunction(position, uv0)
    hull HullFunction
    domain DomainFunction
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main terrain-main-material-for-geometry-pass Terrain

    target s0 DiffuseRGB float3
    target s1 WorldNormalXYZ_UnusedA float4
    target s2 SpecularRGB_ShininessA float4
    target s3 EmissiveRGB float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"
#include "objectMaterial.hlsl"

struct terrain_main_material_for_geometry_pass {
    float3 DiffuseColor;
    float3 SpecularColor;
    float Shininess;
    float TerrainScale;
    float HeightScale;
    float NoiseResolution;
    float Speed;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    terrain_main_material_for_geometry_pass _Terrain;
};
SamplerState InputSampler : register(s1, space2);
Texture2D DiffuseTexture : register(t2, space2);

struct VertexInput {
    float3 Position : POSITION;
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

struct Interpolators {
    float4 Position : SV_POSITION;
    nointerpolation float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float3 WorldPosition : TEXCOORD1;
};

//float Hash(float3 p) {
//    p = frac(p * float3(0.1031, 0.1030, 0.0973));
//    p += dot(p, p.yzx + 19.19);
//    return frac((p.x + p.y) * p.z);
//}

float Hash(float3 p) {
    return frac(sin(dot(p, float3(12.9898, 78.233, 45.164))) * 43758.5453);
}

float Noise(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);

    float3 u = f * f * (3.0 - 2.0 * f);

    float n000 = Hash(i + float3(0, 0, 0));
    float n100 = Hash(i + float3(1, 0, 0));
    float n010 = Hash(i + float3(0, 1, 0));
    float n110 = Hash(i + float3(1, 1, 0));
    float n001 = Hash(i + float3(0, 0, 1));
    float n101 = Hash(i + float3(1, 0, 1));
    float n011 = Hash(i + float3(0, 1, 1));
    float n111 = Hash(i + float3(1, 1, 1));

    float nx0 = lerp(n000, n100, u.x);
    float nx1 = lerp(n010, n110, u.x);
    float nxy0 = lerp(nx0, nx1, u.y);

    float nx0z = lerp(n001, n101, u.x);
    float nx1z = lerp(n011, n111, u.x);
    float nxy1 = lerp(nx0z, nx1z, u.y);

    return lerp(nxy0, nxy1, u.z);
}

float fbm(float3 p, int octaves) {
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float maxValue = 0.0;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * Noise(p * frequency);
        maxValue += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }

    return value / maxValue;
}

float quadstep (float x) {
    return x > 0.5 ? 1 - (-2*x+2)*(-2*x+2)/2 : 2*x*x;
}

float terrainFunc (float2 uv, float2 noiseUV) {
    float3 samplePos = float3(noiseUV, 0.0);
    float noiseValue = fbm(samplePos, 4);
    
    float2 centre = float2(0.5, 0.5);
    float innerRadius = 0.1;
    float outerRadius = 0.4;
    float dist = distance(uv, centre);
    float t = saturate((dist - innerRadius) / (outerRadius - innerRadius));
    //t = smoothstep(0.0, 1.0, t);
    t = quadstep(t);

    float halfRange = 0.5 * t;
    float finalHeight = 0.5 - halfRange + halfRange * 2.f * noiseValue;
    //float finalHeight = lerp(0.5, terrainHeight, t);

    return finalHeight;
}

Interpolators VertexFunction(VertexInput input) {
    float2 terrainUV = input.UV;
    terrainUV *= _Terrain.NoiseResolution;
    terrainUV += (_Frame.Time * _Terrain.Speed).rr;
    float terrainHeight = terrainFunc(input.UV, terrainUV) - 0.5;

    float3 displacedPos = input.Position * float3(_Terrain.TerrainScale, 1.0, _Terrain.TerrainScale);
    displacedPos.y += terrainHeight * _Terrain.HeightScale;

    float4 worldPosition = mul(float4(displacedPos, 1.0), _GeometryObject.Model);

    Interpolators output;
    output.Position = mul(worldPosition, _GeometryObject.ProjectionView);
    output.Normal = float3(0, 0, 0); // normal is computed in hull
    output.UV = input.UV;
    output.WorldPosition = worldPosition.xyz;

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
    output.UV = barycentric.x * patch[0].UV + barycentric.y * patch[1].UV + barycentric.z * patch[2].UV;

    output.Normal = normalize(output.Normal);

    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float4 diffuseColor = DiffuseTexture.Sample(InputSampler, input.UV);

    PixelOutput output;
    output.DiffuseRGB.rgb = diffuseColor.rgb * _Terrain.DiffuseColor;
    output.WorldNormalXYZ_UnusedA.xyz = normalize(input.Normal);
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = _Terrain.SpecularColor;
    output.SpecularRGB_ShininessA.a = _Terrain.Shininess / 2048.0;
    output.EmissiveRGB = float3(0, 0, 0);

    return output;
}
