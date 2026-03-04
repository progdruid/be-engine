/*

@be-material: tesselated-main-material-for-geometry-pass
[
    "DiffuseColor: float3 = [0.28, 0.39, 1.0]",
    "SpecularColor: float3 = [1.0, 1.0, 1.0]",
    "Shininess: float = 100.0",
    "DiffuseTexture: texture2d(0) = white",
    "TessellationLevel: float = 1024.0",
    "DisplacementStrength: float = -1.0",
    "AnimationSpeed: float = 1.5",
    "NoiseFrequency: float = 30.0",

    "InputSampler: sampler(0) = point-clamp",
]
@be-end

@be-shader: tessellated
{
    "topology": "patch-list-3",
    "vertex": "VertexFunction",
    "vertexLayout": ["position", "normal", "uv0"],
    "tesselation": {
        "hull": "HullFunction",
        "domain": "DomainFunction"
    },
    "pixel": "PixelFunction",
    "materials": {
        "geometry-object": { "scheme": "object-material-for-geometry-pass", "slot": 1, "var": "Object" },
        "geometry-main": { "scheme": "tesselated-main-material-for-geometry-pass", "slot": 2, "var": "Tesselated" },
    },
    "targets": {
        "DiffuseRGB":             { "type": "float3", "slot": 0 },
        "WorldNormalXYZ_UnusedA": { "type": "float4", "slot": 1 },
        "SpecularRGB_ShininessA": { "type": "float4", "slot": 2 },
    },
}
@be-end
*/

/*========================================================*/
// region @be-auto-boilerplate
#include "objectMaterial.hlsl"

struct tesselated_main_material_for_geometry_pass {
    float3 DiffuseColor;
    float3 SpecularColor;
    float Shininess;
    float TessellationLevel;
    float DisplacementStrength;
    float AnimationSpeed;
    float NoiseFrequency;
};

Texture2D DiffuseTexture : register(t0);
SamplerState InputSampler : register(s0);

cbuffer CBuffer1 : register(b1) {
    object_material_for_geometry_pass _Object;
};

cbuffer CBuffer2 : register(b2) {
    tesselated_main_material_for_geometry_pass _Tesselated;
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
};

// endregion
/*========================================================*/

#include <BeUniformBuffer.hlsli>
struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float3 WorldPosition : TEXCOORD1;
};


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

float GetDisplacement(float3 worldPos) {
    float distFromOrigin = length(worldPos.xz);
    float ripple = sin(distFromOrigin * 3.0 + _Time * _Tesselated.AnimationSpeed * 2.0) * 0.5 + 0.5;

    float3 noisePos = worldPos * _Tesselated.NoiseFrequency + _Time * _Tesselated.AnimationSpeed * float3(0.3, 0.5, 0.7);
    float fbmVal = fbm(noisePos, 2);

    float result = lerp(fbmVal - 0.5, ripple, 0.6);
    result*=result*result;
    
    return result;
}

float3 GetDisplacedPos (const OutputPatch<VertexOutput, 3> patch, float3 bary, float3 objectCenter) {
    float3 worldPos = bary.x * patch[0].WorldPosition +
                      bary.y * patch[1].WorldPosition +
                      bary.z * patch[2].WorldPosition;

    float3 dirFromCenter = normalize(worldPos - objectCenter);
    float disp = GetDisplacement(worldPos);
    float3 displacedPos = worldPos + dirFromCenter * disp * _Tesselated.DisplacementStrength;
    return displacedPos;
}

VertexOutput VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _Object.Model);

    VertexOutput output;
    output.Position = float4(0, 0, 0, 1);
    output.Normal = float3(0, 0, 0);
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

    float factor = _Tesselated.TessellationLevel;

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
    float3 objectCenter = mul(float4(-0.5, -0.5, 0.5, 1), _Object.Model).xyz;
    
    float epsilon = 0.0001;
    float3 bary_du = float3(barycentric.x + epsilon, barycentric.y - epsilon, barycentric.z);
    float3 bary_dv = float3(barycentric.x, barycentric.y + epsilon, barycentric.z - epsilon);

    float3 displacedPos = GetDisplacedPos(patch, barycentric, objectCenter);
    float3 displacedPos_u = GetDisplacedPos(patch, bary_du, objectCenter);
    float3 displacedPos_v = GetDisplacedPos(patch, bary_dv, objectCenter);

    float3 tangentU = displacedPos_u - displacedPos;
    float3 tangentV = displacedPos_v - displacedPos;

    float3 normal = -normalize(cross(tangentV, tangentU));

    float2 uv = barycentric.x * patch[0].UV +
                barycentric.y * patch[1].UV +
                barycentric.z * patch[2].UV;
        
    VertexOutput output;
    output.WorldPosition = displacedPos;
    output.Position = mul(float4(displacedPos, 1.0), _Object.ProjectionView);
    output.Normal = normal;
    output.UV = uv;

    return output;
}

PixelOutput PixelFunction(VertexOutput input) {
    float4 diffuseColor = DiffuseTexture.Sample(InputSampler, input.UV);

    PixelOutput output;
    output.DiffuseRGB = diffuseColor.rgb * _Tesselated.DiffuseColor;
    output.WorldNormalXYZ_UnusedA.xyz = normalize(input.Normal);
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = _Tesselated.SpecularColor;
    output.SpecularRGB_ShininessA.a = _Tesselated.Shininess / 2048.0;

    return output;
}
