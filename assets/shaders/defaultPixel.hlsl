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

    float ambient = AmbientColor * 0.1f;

    float sunIntensity = saturate(dot(normal, -DirectionalLightVector));
    float diffuse = sunIntensity * DirectionalLightColor;
    
    float3 viewDirection = normalize(CameraPosition - input.WorldPosition);
    float3 reflectedLightDirection = reflect(DirectionalLightVector, normal);
    float specularFactor = pow(saturate(dot(viewDirection, reflectedLightDirection)), 32);
    float3 specular = specularFactor * DirectionalLightColor * 0.5f;
    
    color.rgb *= (ambient + diffuse + specular);
    
    return color;
};