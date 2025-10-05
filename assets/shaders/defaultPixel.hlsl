#include <BeInclude.hlsli>

struct PixelInput {
    float4 Position : SV_Position;
    float3 WorldPosition : POSITION1;
    float3 Normal : NORMAL;
    float3 Color    : COLOR0;
};

float4 main(PixelInput input) : SV_Target {
    float4 color = float4(input.Color, 1.0f);

    float3 normal = normalize(input.Normal);

    float ambient = _AmbientColor * 0.1f;

    float sunIntensity = saturate(dot(normal, -_DirectionalLightVector));
    float diffuse = sunIntensity * _DirectionalLightColor;
    
    float3 viewDirection = normalize(_CameraPosition - input.WorldPosition);
    float3 reflectedLightDirection = reflect(_DirectionalLightVector, normal);
    float specularFactor = pow(saturate(dot(viewDirection, reflectedLightDirection)), 32);
    float3 specular = specularFactor * _DirectionalLightColor * 0.5f;
    
    color.rgb *= (ambient + diffuse + specular);
    
    return color;
};