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
    "Diffuse_RGB_or_Albedo_RGB: texture2d = black",
    "WorldNormal_XYZ_LMF_W: texture2d = black",
    "SpecShin_RGBA_or_MRAO_RGB: texture2d = black",
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
        "frame": { "scheme": "uniform-material", "slot": 0 },
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
#include "uniform-material.hlsl"

struct point_light_material {
    float3 Position;
    float Radius;
    float3 Color;
    float Power;
    float HasShadowMap;
    float ShadowMapResolution;
    float ShadowNearPlane;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    point_light_material _PointLight;
};
SamplerState InputSampler : register(s1, space1);
Texture2D Depth : register(t2, space1);
Texture2D Diffuse_RGB_or_Albedo_RGB : register(t3, space1);
Texture2D WorldNormal_XYZ_LMF_W : register(t4, space1);
Texture2D SpecShin_RGBA_or_MRAO_RGB : register(t5, space1);
TextureCube PointLightShadowMap : register(t6, space1);

struct PixelOutput {
    float3 LightHDR : SV_Target0;
};

// endregion
/*========================================================*/

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
    float3 diffuse_albedo               = Diffuse_RGB_or_Albedo_RGB.Sample(InputSampler, input.UV).rgb;
    float4 worldNormal_lightModelFlag   = WorldNormal_XYZ_LMF_W.Sample(InputSampler, input.UV).xyzw;
    float4 specShin_mrao                = SpecShin_RGBA_or_MRAO_RGB.Sample(InputSampler, input.UV).rgba;

    float3 worldPos = ReconstructWorldPosition(input.UV, depth, _Frame.CameraInverseProjectionView);
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
    
    float3 viewVec = _Frame.CameraPosition - worldPos;
    float3 lit; 
    if (worldNormal_lightModelFlag.w < 0.5) {
        lit = StandardLambertBlinnPhong(
            worldNormal_lightModelFlag.xyz,
            viewVec,
            lightDir,
            //_AmbientColor,
            _PointLight.Color,
            _PointLight.Power * attenuation,
            diffuse_albedo.rgb,
            specShin_mrao.rgb,
            specShin_mrao.a
        );
    } else {
        lit = StandardPBR(
            worldNormal_lightModelFlag.xyz,
            viewVec,
            lightDir,
            _PointLight.Color * _PointLight.Power,
            diffuse_albedo.rgb,
            specShin_mrao.rgb
        );
    }

    PixelOutput output;
    output.LightHDR = lit * shadowAbsenceFactor;
    return output;
}
