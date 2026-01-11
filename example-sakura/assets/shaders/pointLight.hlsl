/*

@be-material: point-light-material
[
    "Position: float3 = [0, 0, 0]",
    "Radius: float = 0",
    "Color: float3 = [0, 0, 0]",
    "Power: float = 0",
    "HasShadowMap: float = 0",
    "ShadowMapResolution: float = 0",
    "ShadowNearPlane: float = 0",
    
    "Depth: texture2d(0) = black",
    "Diffuse: texture2d(1) = black",
    "WorldNormal: texture2d(2) = black",
    "Specular_Shininess: texture2d(3) = black",
    "PointLightShadowMap: texture2d(4) = black",

    "InputSampler: sampler(0) = point-clamp"
]
@be-end

@be-shader: point-light
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "materials": {
        "main": { "scheme": "point-light-material", "slot": 2 },
    },
    "targets": {
        "LightHDR": 0
    }
}
@be-end

*/

#include <BeUniformBuffer.hlsli>
#include <BeFunctions.hlsli>
#include "fullscreen-vertex.hlsl"

Texture2D Depth : register(t0);
Texture2D DiffuseRGB : register(t1);
Texture2D WorldNormalXYZ_UnusedA : register(t2);
Texture2D SpecularRGB_ShininessA : register(t3);
TextureCube PointLightShadowMap : register(t4);
SamplerState InputSampler : register(s0);

cbuffer PointLightBuffer: register(b2) {
    float3 _PointLightPosition;
    float _PointLightRadius;
    float3 _PointLightColor;
    float _PointLightPower;
    
    float _PointLightHasShadowMap;
    float _PointLightShadowMapResolution;
    float _PointLightShadowNearPlane;
};

float SamplePointLightShadow(float3 worldPos) {
    float3 lightDir = worldPos - _PointLightPosition;
    float distanceToLight = length(lightDir);

    // sample cubemap with direction
    float3 sampleDir = normalize(lightDir);
    float shadowmapDepth = PointLightShadowMap.Sample(InputSampler, sampleDir).r;
    float near = _PointLightShadowNearPlane;
    float far = _PointLightRadius;
    float linearDepth = (near * far) / (far - shadowmapDepth * (far - near));
    
    float xCos = abs(dot(sampleDir, float3(1, 0, 0)));
    float yCos = abs(dot(sampleDir, float3(0, 1, 0)));
    float zCos = abs(dot(sampleDir, float3(0, 0, 1)));

    float cos = max(xCos, max(yCos, zCos));

    float shadowmapDistance = linearDepth / cos; 
    
    float shadow = (distanceToLight - 0.02) < shadowmapDistance ? 1.0 : 0.0;
    return shadow;
}



float3 PixelFunction(FullscreenVSOutput input) : SV_TARGET {
    float depth = Depth.Sample(InputSampler, input.UV).r;
    float3 diffuse = DiffuseRGB.Sample(InputSampler, input.UV);
    float3 worldNormal = WorldNormalXYZ_UnusedA.Sample(InputSampler, input.UV).xyz;
    float4 specular_shininess = SpecularRGB_ShininessA.Sample(InputSampler, input.UV);

    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _CameraInverseProjectionView);
    float3 lightDir = _PointLightPosition - worldPos;
    float distanceToLight = length(lightDir);
    if (distanceToLight > _PointLightRadius) {
        discard;
    }

    float shadowAbsenceFactor = 1.0;
    if (_PointLightHasShadowMap > 0.5) {
        shadowAbsenceFactor = SamplePointLightShadow(worldPos);
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
        specular_shininess.a
    );

    return lit * shadowAbsenceFactor;
}
