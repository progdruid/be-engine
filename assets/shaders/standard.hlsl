
/*
@be-shader-header
{
    "vertex": "VertexFunction",
    "pixel": "PixelFunction",
    "vertexLayout": ["position", "normal", "uv0"],
    "material": {
        "DiffuseColor": { "type": "float3", "default": [1.0, 1.0, 1.0] },
        "SpecularColor0": { "type": "float3", "default": [1.0, 1.0, 1.0] },
        "Shininess0": { "type": "float", "default": -1.0 },
        "SpecularColor1": { "type": "float3", "default": [1.0, 1.0, 1.0] },
        "Shininess1": { "type": "float", "default": -1.0 },
        "DiffuseTexture": { "type": "texture2d", "slot": 0, "default": "white" },
        "SpecularTexture": { "type": "texture2d", "slot": 1, "default": "black" }
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
    float3 _SpecularColor1;
    float _Shininess1;
};

SamplerState DefaultSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
Texture2D Specular : register(t1);

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV    : TEXCOORD0;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV    : TEXCOORD0;

    float3 ViewDirection : TEXCOORD1;
};

struct PixelOutput {
    float4 DiffuseRGBA : SV_Target0;
    float4 WorldNormalXYZ_UnusedA : SV_Target1;
    float4 SpecularRGB_ShininessA : SV_Target2;
};

VertexOutput VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _Model);

    VertexOutput output;
    output.Position = mul(worldPosition, _ProjectionView);
    output.ViewDirection = _CameraPosition - worldPosition.xyz;
    output.Normal = normalize(mul(input.Normal, (float3x3)_Model));
    output.UV = input.UV;

    return output;
}

PixelOutput PixelFunction(VertexOutput input) {
    float4 diffuseColor = DiffuseTexture.Sample(DefaultSampler, input.UV);
    float4 specularColor = Specular.Sample(DefaultSampler, input.UV);
    if (diffuseColor.a < 0.5) discard;

    PixelOutput output;
    output.DiffuseRGBA.rgb = diffuseColor.rgb * _DiffuseColor;
    output.DiffuseRGBA.a = 1.0;
    output.WorldNormalXYZ_UnusedA.xyz = normalize(input.Normal);
    output.WorldNormalXYZ_UnusedA.w = 1.0;
    output.SpecularRGB_ShininessA.rgb = specularColor.rgb * _SpecularColor0;
    output.SpecularRGB_ShininessA.a = _Shininess0;
    
    return output;
};