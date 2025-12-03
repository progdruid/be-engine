/*
@be-shader-header
{
    "vertex": "VertexFunction",
    "vertexLayout": ["position", "normal", "uv0"],
    "tesselation": {
        "hull": "HullFunction",
        "domain": "DomainFunction"
    },
    "pixel": "PixelFunction",
    "material": {
        "DiffuseColor": { "type": "float3", "default": [0.5, 0.5, 0.5] },
        "SpecularColor": { "type": "float3", "default": [-0.2, -0.2, -0.1] }, 
        "Shininess":  { "type": "float", "default": 0.0 },
        "DiffuseTexture": { "type": "texture2d", "slot": 0, "default": "white" },
        "TerrainScale": { "type": "float", "default": 1.0 },
        "HeightScale": { "type": "float", "default": 1.0 },
        "NoiseResolution": { "type": "float", "default": 4.0 },
        "Speed": { "type": "float", "default": 0.2 }
    }
}
@be-shader-header-end
*/

#include <BeUniformBuffer.hlsli>

cbuffer ModelBuffer: register(b1) {
    row_major float4x4 _Model;
}

cbuffer MaterialBuffer: register(b2) {
    float3 _DiffuseColor;
    float3 _SpecularColor;
    float _Shininess;
    float _TerrainScale;
    float _HeightScale;
    float _NoiseResolution;
    float _Speed;
};

SamplerState DefaultSampler : register(s0);
Texture2D DiffuseTexture : register(t0);

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    nointerpolation float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float3 WorldPosition : TEXCOORD1;
};

struct PixelOutput {
    float4 DiffuseRGBA : SV_Target0;
    float4 WorldNormalXYZ_UnusedA : SV_Target1;
    float4 SpecularRGB_ShininessA : SV_Target2;
};

// Hash function for pseudo-random values
float Hash(float3 p) {
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yzx + 19.19);
    return frac((p.x + p.y) * p.z);
}

// Improved Perlin-like noise function using value noise
float Noise(float3 p) {
    // Hash-based pseudo-random
    float3 i = floor(p);
    float3 f = frac(p);

    // Smoothstep curve for interpolation
    float3 u = f * f * (3.0 - 2.0 * f);

    // Sample corners of the cube
    float n000 = Hash(i + float3(0, 0, 0));
    float n100 = Hash(i + float3(1, 0, 0));
    float n010 = Hash(i + float3(0, 1, 0));
    float n110 = Hash(i + float3(1, 1, 0));
    float n001 = Hash(i + float3(0, 0, 1));
    float n101 = Hash(i + float3(1, 0, 1));
    float n011 = Hash(i + float3(0, 1, 1));
    float n111 = Hash(i + float3(1, 1, 1));

    // Interpolate
    float nx0 = lerp(n000, n100, u.x);
    float nx1 = lerp(n010, n110, u.x);
    float nxy0 = lerp(nx0, nx1, u.y);

    float nx0z = lerp(n001, n101, u.x);
    float nx1z = lerp(n011, n111, u.x);
    float nxy1 = lerp(nx0z, nx1z, u.y);

    return lerp(nxy0, nxy1, u.z);
}

// Fractional Brownian Motion - multiple octaves of noise
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

VertexOutput VertexFunction(VertexInput input) {
    // Sample noise in 0-1 UV space for terrain displacement (sampling unchanged)
    float2 terrainUV = input.UV;
    terrainUV *= _NoiseResolution;
    terrainUV += (_Time * _Speed).rr;
    float terrainHeight = terrainFunc(input.UV, terrainUV) - 0.5;

    // Apply scales to vertex positions
    float3 displacedPos = input.Position * float3(_TerrainScale, 1.0, _TerrainScale);
    displacedPos.y += terrainHeight * _HeightScale;

    // Transform to world space
    float4 worldPosition = mul(float4(displacedPos, 1.0), _Model);

    // Calculate normals using finite differences in UV space
    // Account for aspect ratio: when horizontal and vertical scales differ,
    // the slope gradient must account for this ratio
    float epsilon = 0.01;
    float h_xp = terrainFunc(input.UV, terrainUV + float2(epsilon, 0)) * _HeightScale;
    float h_xn = terrainFunc(input.UV, terrainUV - float2(epsilon, 0)) * _HeightScale;
    float h_zp = terrainFunc(input.UV, terrainUV + float2(0, epsilon)) * _HeightScale;
    float h_zn = terrainFunc(input.UV, terrainUV - float2(0, epsilon)) * _HeightScale;

    float3 normal;
    // Gradient accounts for scaling: height difference / horizontal distance
    // Horizontal distance in world space = epsilon * TerrainScale
    normal.x = (h_xn - h_xp) / (2.0 * epsilon * _TerrainScale);
    normal.y = 1.0;
    normal.z = (h_zn - h_zp) / (2.0 * epsilon * _TerrainScale);
    normal = normalize(normal);

    VertexOutput output;
    output.Position = mul(worldPosition, _ProjectionView);
    output.Normal = normalize(mul(normal, (float3x3)_Model));
    output.UV = input.UV;
    output.WorldPosition = worldPosition.xyz;

    return output;
}

// Patch constant output structure
struct PatchConstantOutput {
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};

// Patch constant function for hull shader
PatchConstantOutput PatchConstantFunction(InputPatch<VertexOutput, 3> patch) {
    PatchConstantOutput output;

    // Pass-through: no tessellation
    output.EdgeTessFactor[0] = 1.0f;
    output.EdgeTessFactor[1] = 1.0f;
    output.EdgeTessFactor[2] = 1.0f;
    output.InsideTessFactor = 1.0f;

    return output;
}

// Hull shader
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunction")]
VertexOutput HullFunction(InputPatch<VertexOutput, 3> patch, uint pointId : SV_OutputControlPointID) {
    VertexOutput output = patch[pointId];

    // Calculate face normal from the three displaced positions
    float3 v0 = patch[0].WorldPosition;
    float3 v1 = patch[1].WorldPosition;
    float3 v2 = patch[2].WorldPosition;
    output.Normal = normalize(cross(v1 - v0, v2 - v0));

    return output;
}

// Domain shader
[domain("tri")]
VertexOutput DomainFunction(PatchConstantOutput patchData, float3 barycentric : SV_DomainLocation, const OutputPatch<VertexOutput, 3> patch) {
    VertexOutput output;

    // Interpolate using barycentric coordinates
    output.Position = barycentric.x * patch[0].Position + barycentric.y * patch[1].Position + barycentric.z * patch[2].Position;
    output.Normal = barycentric.x * patch[0].Normal + barycentric.y * patch[1].Normal + barycentric.z * patch[2].Normal;
    output.UV = barycentric.x * patch[0].UV + barycentric.y * patch[1].UV + barycentric.z * patch[2].UV;
    output.WorldPosition = barycentric.x * patch[0].WorldPosition + barycentric.y * patch[1].WorldPosition + barycentric.z * patch[2].WorldPosition;

    // Normalize interpolated vectors
    output.Normal = normalize(output.Normal);

    return output;
}

PixelOutput PixelFunction(VertexOutput input) {
    float4 diffuseColor = DiffuseTexture.Sample(DefaultSampler, input.UV);

    PixelOutput output;
    output.DiffuseRGBA.rgb = diffuseColor.rgb * _DiffuseColor;
    output.DiffuseRGBA.a = 1.0;
    output.WorldNormalXYZ_UnusedA.xyz = normalize(input.Normal);
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = _SpecularColor;
    output.SpecularRGB_ShininessA.a = _Shininess / 2048.0;

    return output;
}
