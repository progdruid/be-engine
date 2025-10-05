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

    //should be in material buffer
    const float3 SpecularColor = float3(0.99, 0.99, 0.99) * 0.4;
    const float Shininess = 32.0;
    const float3 SpecularColor2 = float3(0.99, 0.99, 0.99) * 0.9;
    const float Shininess2 = 1024.0;
    
    float3 normal = normalize(input.Normal);
    float3 viewDir = normalize(input.ViewDirection);
    float3 lightDir = normalize(-_DirectionalLightVector);
    float diffuseValue = saturate(dot(normal, lightDir));
    float specularValue = 0.0;
    float specularValue2 = 0.0;
    if (Shininess > 0) {
        float3 reflectDir = normalize(reflect(-lightDir, normal));
        specularValue = pow(saturate(dot(viewDir, reflectDir)), Shininess);
        specularValue2 = pow(saturate(dot(viewDir, reflectDir)), Shininess2);

        
        //float3 halfVector = normalize(lightDir + viewDir);
        //specularValue = pow(saturate(dot(normal, halfVector)), Shininess);
    }
    
    float3 light = _DirectionalLightColor * _DirectionalLightPower;
    float3 colorLinear =
        _AmbientColor +
        diffuseColor * diffuseValue * light +
        SpecularColor * specularValue * light +
        SpecularColor2 * specularValue2 * light;

    return float4(colorLinear, 1.0);
};