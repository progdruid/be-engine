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
    
    "Depth: texture2d = black",
    "Diffuse: texture2d = black",
    "WorldNormal: texture2d = black",
    "Specular_Shininess: texture2d = black",
    "PointLightShadowMap: textureCube = black",

    "InputSampler: sampler = point-clamp"
]
@be-end

@be-shader: point-light
{
    "topology": "triangle-strip",
    "vertex": "FullscreenVertexKernel",
    "pixel": "PixelFunction",
    "rasterizer": "back-solid",
    "blend": "additive",
    "depthStencil": "disable",
    "materials": {
        "main": { "scheme": "point-light-material", "slot": 1, "var": "PointLight" },
    },
    "targets": {
        "LightHDR": { "type": "float3", "slot": 0 }
    }
}
@be-end

*/

/*========================================================*/
// region @be-auto-boilerplate
struct point_light_material {
    float3 Position;
    float Radius;
    float3 Color;
    float Power;
    float HasShadowMap;
    float ShadowMapResolution;
    float ShadowNearPlane;
};

cbuffer CBuffer_1 : register(b0, space1) {
    point_light_material _PointLight;
};
SamplerState InputSampler : register(s1, space1);
Texture2D Depth : register(t2, space1);
Texture2D Diffuse : register(t3, space1);
Texture2D WorldNormal : register(t4, space1);
Texture2D Specular_Shininess : register(t5, space1);
TextureCube PointLightShadowMap : register(t6, space1);

struct PixelOutput {
    float3 LightHDR : SV_Target0;
};

// endregion
/*========================================================*/

#include <BeUniformBuffer.hlsli>
#include <BeFunctions.hlsli>
#include "fullscreen-vertex.hlsl"

float SamplePointLightShadow(float3 worldPos) {
    float3 lightDir = worldPos - _PointLight.Position;
    float distanceToLight = length(lightDir);

    // sample cubemap with direction
    float3 sampleDir = normalize(lightDir);
    float shadowmapDepth = PointLightShadowMap.Sample(InputSampler, sampleDir).r;
    float near = _PointLight.ShadowNearPlane;
    float far = _PointLight.Radius;
    float linearDepth = (near * far) / (far - shadowmapDepth * (far - near));
    
    float xCos = abs(dot(sampleDir, float3(1, 0, 0)));
    float yCos = abs(dot(sampleDir, float3(0, 1, 0)));
    float zCos = abs(dot(sampleDir, float3(0, 0, 1)));

    float cos = max(xCos, max(yCos, zCos));

    float shadowmapDistance = linearDepth / cos; 
    
    float shadow = (distanceToLight - 0.02) < shadowmapDistance ? 1.0 : 0.0;
    return shadow;
}



PixelOutput PixelFunction(FullscreenVSOutput input) {
    float depth = Depth.Sample(InputSampler, input.UV).r;
    float3 diffuse = Diffuse.Sample(InputSampler, input.UV).rgb;
    float3 worldNormal = WorldNormal.Sample(InputSampler, input.UV).xyz;
    float4 specular_shininess = Specular_Shininess.Sample(InputSampler, input.UV).rgba;

    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _CameraInverseProjectionView);
    float3 lightDir = _PointLight.Position - worldPos;
    float distanceToLight = length(lightDir);
    if (distanceToLight > _PointLight.Radius) {
        discard;
    }

    float shadowAbsenceFactor = 1.0;
    if (_PointLight.HasShadowMap > 0.5) {
        shadowAbsenceFactor = SamplePointLightShadow(worldPos);
    }
    
    float attenuation = saturate(1.0 - (distanceToLight / _PointLight.Radius));
    attenuation *= attenuation;
    
    float3 viewVec = _CameraPosition - worldPos;
    float3 lit = StandardLambertBlinnPhong(
        worldNormal,
        viewVec,
        lightDir,
        //_AmbientColor,
        _PointLight.Color,
        _PointLight.Power * attenuation,
        diffuse.rgb,
        specular_shininess.rgb,
        specular_shininess.a
    );

    PixelOutput output;
    output.LightHDR = lit * shadowAbsenceFactor;
    return output;
}
