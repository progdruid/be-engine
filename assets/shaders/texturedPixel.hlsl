#include <BeInclude.hlsli>

Texture2D texture0 : register(t0);
SamplerState samp : register(s0);

struct PixelInput {
    float4 Position : SV_Position;
    float3 WorldPosition : POSITION1;
    float3 Normal : NORMAL;
    float2 UV    : TEXCOORD0;
};

float4 main(PixelInput input) : SV_Target {
    float4 color = texture0.Sample(samp, input.UV);
    if (color.a < 0.5) discard;
    
    float3 normal = normalize(input.Normal);

    float3 ambient = AmbientColor * AmbientIntensity;
    
    float sunIntensity = saturate(dot(normal, -DirectionalLightVector.xyz));
    
    float3 diffuse = DirectionalLightColor * sunIntensity;
    
    float3 viewDirection = normalize(CameraPosition - input.WorldPosition);
    float3 reflectedLightDirection = reflect(DirectionalLightVector, normal);
    float specularFactor = pow(saturate(dot(viewDirection, reflectedLightDirection)), 32);
    float3 specular = specularFactor * DirectionalLightColor * 0.5f;
    
    color.rgb *= (ambient + diffuse + specular);
    
    return color;
};