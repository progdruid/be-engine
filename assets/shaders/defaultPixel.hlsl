#include <BeInclude.hlsli>

struct PixelInput {
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float3 Color    : COLOR0;
};

float4 main(PixelInput input) : SV_Target {
    float4 color = float4(input.Color, 1.0f);

    float ambient = AmbientColor * 0.1f;

    float3 normal = normalize(input.Normal);
    float sunIntensity = saturate(dot(normal, -DirectionalLightVector));
    float diffuse = sunIntensity * DirectionalLightColor;
    
    color.rgb *= (ambient + diffuse);
    
    return color;
};