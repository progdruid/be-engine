
/*

@be-material: checkerboard-material-for-geometry-pass
[
    "DiffuseColor: float3 = [1.0, 1.0, 1.0]",
    "SpecularColor: float3 = [0.3, 0.3, 0.3]",
    "Shininess: float = 0.0",
    "TileScale: float = 1.0",

    "DiffuseTexture: texture2d(0) = black",
    "SpecularTexture: texture2d(1) = black",

    "InputSampler: sampler(0) = point-wrap",
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
        "Diffuse.RGB": 0,
        "WorldNormal.XYZ": 1,
        "Specular.RGB_Shininess.A": 2
    }
}
@be-end

*/

#include "objectMaterial.hlsl"

cbuffer ModelBuffer: register(b1) {
    StandardObjectData _Object;
};

cbuffer MaterialBuffer: register(b2) {
    float3 _DiffuseColor;
    float3 _SpecularColor;
    float _Shininess;
    float _TileScale;
};

SamplerState InputSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
Texture2D SpecularTexture : register(t1);

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct PixelOutput {
    float3 DiffuseRGB : SV_Target0;
    float4 WorldNormalXYZ_UnusedA : SV_Target1;
    float4 SpecularRGB_ShininessA : SV_Target2;
};

VertexOutput VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _Object.Model);

    VertexOutput output;
    output.Position = mul(worldPosition, _Object.ProjectionView);
    output.WorldPosition = worldPosition.xyz;
    output.Normal = normalize(mul(input.Normal, (float3x3)_Object.Model));

    return output;
}

PixelOutput PixelFunction(VertexOutput input) {
    input.Normal = normalize(input.Normal);
    
    // Triplanar mapping using world coordinates
    // This ensures texture stays consistent regardless of object scale/transform
    float3 worldPos = input.WorldPosition * _TileScale;
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
    output.DiffuseRGB = triplanarDiffuse.rgb * _DiffuseColor;
    output.WorldNormalXYZ_UnusedA.xyz = input.Normal;
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = triplanarSpecular * _SpecularColor;
    output.SpecularRGB_ShininessA.a = _Shininess / 2048.0;

    return output;
};
