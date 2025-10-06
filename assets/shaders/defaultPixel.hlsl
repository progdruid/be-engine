#include <BeInclude.hlsli>

struct PixelInput {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 Color    : COLOR0;

    float3 ViewDirection : TEXCOORD1;
};

float4 main(PixelInput input) : SV_Target {
    
    float4 diffuseColor = float4(input.Color, 1.0f);
    if (diffuseColor.a < 0.5) discard;
    
    //should be in material buffer
    const float3 SpecularColor0 = float3(0.99, 0.99, 0.99) * 0.4;
    const float Shininess0 = 32.0;
    const float3 SpecularColor1 = float3(0.99, 0.99, 0.99) * 0.9;
    const float Shininess1 = 1024.0;
    
    float3 color =
        StandardLambertBlinnPhong(
            input.Normal,
            input.ViewDirection,
            -_DirectionalLightVector,
            _AmbientColor,
            _DirectionalLightColor,
            _DirectionalLightPower,
            diffuseColor.rgb,
            SpecularColor0,
            SpecularColor1,
            Shininess0,
            Shininess1
        );
    
    return float4(color, 1.0);
};