#include <BeInclude.hlsli>

struct PixelInput {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;

    float3 ViewDirection : TEXCOORD1;
};

float4 main(PixelInput input) : SV_Target {
    float4 diffuseColor = float4(_DiffuseColor, 1.0f);
    
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