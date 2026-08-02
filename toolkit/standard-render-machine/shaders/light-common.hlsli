#include "BeFunctions.hlsli"

struct BeSurfacePoint {
    float3 WorldPosition;
    float3 Normal;
    float3 ViewVector;
    float3 Albedo;
    float3 Surface;
    float SurfaceAlpha;
    bool UsePhong;
};

BeSurfacePoint MakeSurfacePoint(
    float3 worldPosition,
    float3 cameraPosition,
    float4 normalAndFlag,
    float3 albedo,
    float4 surface
) {
    BeSurfacePoint result;
    result.WorldPosition = worldPosition;
    result.Normal = normalAndFlag.xyz;
    result.ViewVector = cameraPosition - worldPosition;
    result.Albedo = albedo;
    result.Surface = surface.rgb;
    result.SurfaceAlpha = surface.a;
    result.UsePhong = normalAndFlag.w > 0.5;
    return result;
}

float3 ShadeLight(BeSurfacePoint surfacePoint, float3 lightVector, float3 color, float power) {
    if (surfacePoint.UsePhong) {
        return StandardLambertBlinnPhong(
            surfacePoint.Normal, surfacePoint.ViewVector, lightVector,
            color, power,
            surfacePoint.Albedo, surfacePoint.Surface, surfacePoint.SurfaceAlpha
        );
    }
    return StandardPBR(
        surfacePoint.Normal, surfacePoint.ViewVector, lightVector,
        color * power,
        surfacePoint.Albedo, surfacePoint.Surface
    );
}

float3 ShadeDirectionalLight(BeSurfacePoint surfacePoint, float3 direction, float3 color, float power) {
    return ShadeLight(surfacePoint, -direction, color, power);
}

float PointLightAttenuation(float distanceToLight, float radius) {
    float attenuation = saturate(1.0 - (distanceToLight / radius));
    return attenuation * attenuation;
}

float3 ShadePointLight(BeSurfacePoint surfacePoint, float3 position, float radius, float3 color, float power) {
    float3 lightVector = position - surfacePoint.WorldPosition;
    float distanceToLight = length(lightVector);
    if (distanceToLight > radius) {
        return (0.0).xxx;
    }
    return ShadeLight(surfacePoint, lightVector, color, power * PointLightAttenuation(distanceToLight, radius));
}
