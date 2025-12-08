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
    "targets": {
        "Diffuse.RGB": 0,
        "WorldNormal.XYZ": 1,
        "Specular.RGB_Shininess.A": 2
    },
    "material": {
        "DiffuseColor": { "type": "float3", "default": [0.2, 0.8, 0.2] },
        "SpecularColor": { "type": "float3", "default": [1.0, 1.0, 1.0] },
        "Shininess": { "type": "float", "default": 64.0 },
        "DiffuseTexture": { "type": "texture2d", "slot": 0, "default": "white" },
        "TessellationLevel": { "type": "float", "default": 64.0 },
        "DisplacementStrength": { "type": "float", "default": 0.5 },
        "AnimationSpeed": { "type": "float", "default": 1.5 },
        "NoiseFrequency": { "type": "float", "default": 4.0 }
    }
}
@be-shader-header-end
*/

#include <BeUniformBuffer.hlsli>

cbuffer ModelBuffer: register(b1) {
    row_major float4x4 _Model;
    row_major float4x4 _ProjectionView;
    float3 _ViewerPosition;
}

cbuffer MaterialBuffer: register(b2) {
    float3 _DiffuseColor;
    float3 _SpecularColor;
    float _Shininess;
    float _TessellationLevel;
    float _DisplacementStrength;
    float _AnimationSpeed;
    float _NoiseFrequency;
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
    float3 WorldPosition : TEXCOORD1;
};

struct PixelOutput {
    float3 DiffuseRGB : SV_Target0;
    float4 WorldNormalXYZ_UnusedA : SV_Target1;
    float4 SpecularRGB_ShininessA : SV_Target2;
};

float Hash(float3 p) {
    return frac(sin(dot(p, float3(12.9898, 78.233, 45.164))) * 43758.5453);
}

float Noise(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = Hash(i);
    float b = Hash(i + float3(1, 0, 0));
    float c = Hash(i + float3(0, 1, 0));
    float d = Hash(i + float3(1, 1, 0));
    float e = Hash(i + float3(0, 0, 1));
    float g = Hash(i + float3(1, 0, 1));
    float h = Hash(i + float3(0, 1, 1));
    float j = Hash(i + float3(1, 1, 1));

    float ab = lerp(a, b, f.x);
    float cd = lerp(c, d, f.x);
    float ef = lerp(e, g, f.x);
    float gh = lerp(h, j, f.x);

    float abcd = lerp(ab, cd, f.y);
    float efgh = lerp(ef, gh, f.y);

    return lerp(abcd, efgh, f.z);
}

float FBM(float3 p, int octaves) {
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

float GetDisplacement(float3 worldPos) {
    float distFromOrigin = length(worldPos.xz);
    float ripple = sin(distFromOrigin * 3.0 + _Time * _AnimationSpeed * 2.0) * 0.5 + 0.5;

    float3 noisePos = worldPos * _NoiseFrequency + _Time * _AnimationSpeed * float3(0.3, 0.5, 0.7);
    float fbm = FBM(noisePos, 2);

    float result = lerp(fbm - 0.5, ripple, 0.6);
    result*=result*result;
    
    return result;
}

VertexOutput VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _Model);

    VertexOutput output;
    output.Position = mul(worldPosition, _ProjectionView);
    output.Normal = normalize(mul(input.Normal, (float3x3)_Model));
    output.UV = input.UV;
    output.WorldPosition = worldPosition.xyz;

    return output;
}

struct PatchConstantOutput {
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};

PatchConstantOutput PatchConstantFunction(InputPatch<VertexOutput, 3> patch) {
    PatchConstantOutput output;

    float factor = _TessellationLevel;

    output.EdgeTessFactor[0] = factor;
    output.EdgeTessFactor[1] = factor;
    output.EdgeTessFactor[2] = factor;
    output.InsideTessFactor = factor;

    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunction")]
VertexOutput HullFunction(InputPatch<VertexOutput, 3> patch, uint pointId : SV_OutputControlPointID) {
    return patch[pointId];
}

[domain("tri")]
VertexOutput DomainFunction(PatchConstantOutput patchData, float3 barycentric : SV_DomainLocation, const OutputPatch<VertexOutput, 3> patch) {
    float3 worldPos = barycentric.x * patch[0].WorldPosition +
                      barycentric.y * patch[1].WorldPosition +
                      barycentric.z * patch[2].WorldPosition;

    float2 uv = barycentric.x * patch[0].UV +
                barycentric.y * patch[1].UV +
                barycentric.z * patch[2].UV;

    float disp = GetDisplacement(worldPos);
    float3 displacedPos = worldPos + float3(0, 1, 0) * disp * _DisplacementStrength;

    float epsilon = 0.0001;

    float3 bary_du = float3(barycentric.x + epsilon, barycentric.y - epsilon, barycentric.z);
    float3 bary_dv = float3(barycentric.x, barycentric.y + epsilon, barycentric.z - epsilon);

    float3 worldPos_u = bary_du.x * patch[0].WorldPosition +
                        bary_du.y * patch[1].WorldPosition +
                        bary_du.z * patch[2].WorldPosition;

    float3 worldPos_v = bary_dv.x * patch[0].WorldPosition +
                        bary_dv.y * patch[1].WorldPosition +
                        bary_dv.z * patch[2].WorldPosition;

    float disp_u = GetDisplacement(worldPos_u);
    float disp_v = GetDisplacement(worldPos_v);

    float3 displacedPos_u = worldPos_u + float3(0, 1, 0) * disp_u * _DisplacementStrength;
    float3 displacedPos_v = worldPos_v + float3(0, 1, 0) * disp_v * _DisplacementStrength;

    float3 tangentU = displacedPos_u - displacedPos;
    float3 tangentV = displacedPos_v - displacedPos;

    float3 normal = -normalize(cross(tangentV, tangentU));

    VertexOutput output;
    output.WorldPosition = displacedPos;
    output.Position = mul(float4(displacedPos, 1.0), _ProjectionView);
    output.Normal = normal;
    output.UV = uv;

    return output;
}

PixelOutput PixelFunction(VertexOutput input) {
    float4 diffuseColor = DiffuseTexture.Sample(DefaultSampler, input.UV);

    PixelOutput output;
    output.DiffuseRGB = diffuseColor.rgb * _DiffuseColor;
    output.WorldNormalXYZ_UnusedA.xyz = normalize(input.Normal);
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = _SpecularColor;
    output.SpecularRGB_ShininessA.a = _Shininess / 2048.0;

    return output;
}
