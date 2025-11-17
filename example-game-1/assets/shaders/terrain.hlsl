/*
@be-shader-header
{
    "vertex": "VertexFunction",
    "pixel": "PixelFunction",
    "vertexLayout": ["position", "normal", "uv0"],
    "material": {
        "DiffuseColor": { "type": "float3", "default": [0.2, 0.6, 0.2] },
        "SpecularColor0": { "type": "float3", "default": [0.1, 0.15, 0.05] },
        "Shininess0": { "type": "float", "default": 0.01 },
        "TerrainScale": { "type": "float", "default": 1.0 },
        "HeightScale": { "type": "float", "default": 1.0 },
        "DiffuseTexture": { "type": "texture2d", "slot": 0, "default": "white" }
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
    float3 _SpecularColor0;
    float _Shininess0;
    float _TerrainScale;
    float _HeightScale;
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
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float3 ViewDirection : TEXCOORD1;
    float3 WorldPosition : TEXCOORD2;
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

float terrainFunc (float2 uv) {
    float3 samplePos = float3(uv, 0.0);
    float terrainHeight = fbm(samplePos, 4) * _HeightScale;

    float2 centre = float2(0.5, 0.5);
    float innerRadius = 0.15;
    float outerRadius = 0.40;
    float dist = distance(uv, centre);
    float t = saturate((dist - innerRadius) / (outerRadius - innerRadius));
    t = smoothstep(0.0, 1.0, t);


    float finalHeight = terrainHeight * t;

    return finalHeight;
}

VertexOutput VertexFunction(VertexInput input) {
    // Sample noise in 0-1 UV space for terrain displacement (sampling unchanged)
    float terrainHeight = terrainFunc(input.UV);

    // Apply scales to vertex positions
    float3 displacedPos = input.Position * float3(_TerrainScale, 1.0, _TerrainScale);
    displacedPos.y += terrainHeight * _HeightScale;

    // Transform to world space
    float4 worldPosition = mul(float4(displacedPos, 1.0), _Model);

    // Calculate normals using finite differences in UV space
    // Account for aspect ratio: when horizontal and vertical scales differ,
    // the slope gradient must account for this ratio
    float epsilon = 0.01;
    float h_xp = terrainFunc(input.UV + float2(epsilon, 0)) * _HeightScale;
    float h_xn = terrainFunc(input.UV - float2(epsilon, 0)) * _HeightScale;
    float h_zp = terrainFunc(input.UV + float2(0, epsilon)) * _HeightScale;
    float h_zn = terrainFunc(input.UV - float2(0, epsilon)) * _HeightScale;

    float3 normal;
    // Gradient accounts for scaling: height difference / horizontal distance
    // Horizontal distance in world space = epsilon * TerrainScale
    normal.x = (h_xn - h_xp) / (2.0 * epsilon * _TerrainScale);
    normal.y = 1.0;
    normal.z = (h_zn - h_zp) / (2.0 * epsilon * _TerrainScale);
    normal = normalize(normal);

    VertexOutput output;
    output.Position = mul(worldPosition, _ProjectionView);
    output.ViewDirection = _CameraPosition - worldPosition.xyz;
    output.Normal = normalize(mul(normal, (float3x3)_Model));
    output.UV = input.UV;
    output.WorldPosition = worldPosition.xyz;

    return output;
}

PixelOutput PixelFunction(VertexOutput input) {
    float4 diffuseColor = DiffuseTexture.Sample(DefaultSampler, input.UV);

    PixelOutput output;
    output.DiffuseRGBA.rgb = diffuseColor.rgb * _DiffuseColor;
    output.DiffuseRGBA.a = 1.0;
    output.WorldNormalXYZ_UnusedA.xyz = normalize(input.Normal);
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = _SpecularColor0;
    output.SpecularRGB_ShininessA.a = _Shininess0;

    return output;
}
