#include <BeUniformBuffer.hlsli>
#include <BeFunctions.hlsli>
#include <BePointLightBuffer.hlsli>

Texture2D Depth : register(t0);
Texture2D DiffuseRGBA : register(t1);
Texture2D WorldNormalXYZ_UnusedA : register(t2);
Texture2D SpecularRGB_ShininessA : register(t3);
SamplerState InputSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float3 main(PSInput input) : SV_TARGET {
    float depth = Depth.Sample(InputSampler, input.UV).r;
    float4 diffuse = DiffuseRGBA.Sample(InputSampler, input.UV);
    float3 worldNormal = WorldNormalXYZ_UnusedA.Sample(InputSampler, input.UV).xyz;
    float4 specular_shininess = SpecularRGB_ShininessA.Sample(InputSampler, input.UV);

    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _InverseProjectionView);
    float3 lightDir = _PointLightPosition - worldPos;
    float distanceToLight = length(lightDir);
    if (distanceToLight > _PointLightRadius) {
        discard;
    }

    float attenuation = saturate(1.0 - (distanceToLight / _PointLightRadius));
    attenuation *= attenuation;
    
    float3 viewVec = _CameraPosition - worldPos;
    float3 lit = StandardLambertBlinnPhong(
        worldNormal,
        viewVec,
        lightDir,
        //_AmbientColor,
        _PointLightColor,
        _PointLightPower * attenuation,
        diffuse.rgb,
        specular_shininess.rgb,
        float3(0, 0, 0),
        specular_shininess.a * 2048.0,
        0.f
    );

    return lit;
}
