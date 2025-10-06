#include <BeInclude.hlsli>

Texture2D Texture0 : register(t0);
SamplerState Samp : register(s0);

struct PixelInput {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV    : TEXCOORD0;

    float3 ViewDirection : TEXCOORD1;
};

float4 main(PixelInput input) : SV_Target {
    float4 diffuseColor = Texture0.Sample(Samp, input.UV);
    if (diffuseColor.a < 0.5) discard;
    
    float3 color =
        StandardLambertBlinnPhong(
            input.Normal,
            input.ViewDirection,
            -_DirectionalLightVector,
            _AmbientColor,
            _DirectionalLightColor,
            _DirectionalLightPower,
            diffuseColor.rgb,
            _SpecularColor0,
            _SpecularColor1,
            _Shininess0,
            _Shininess1
        );
    
    return float4(color, 1.0);
};