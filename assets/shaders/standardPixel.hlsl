
#include <BeMaterialBuffer.hlsli>

struct PixelInput {
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

PixelOutput main(PixelInput input) {
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