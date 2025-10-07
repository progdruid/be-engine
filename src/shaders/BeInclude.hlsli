
#define PI 3.14159265359

cbuffer UniformBuffer: register(b0) {
    row_major float4x4 _ViewProjection;
    float3 _CameraPosition;
    
    float3 _AmbientColor;
    float3 _DirectionalLightVector;
    float3 _DirectionalLightColor;
    float _DirectionalLightPower;
};

cbuffer MaterialBuffer: register(b1) {
    row_major float4x4 _Model;
    
    float3 _DiffuseColor;
    float3 _SpecularColor0;
    float _Shininess0;
    float3 _SpecularColor1;
    float _Shininess1;
};

SamplerState DefaultSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
Texture2D Specular : register(t1);


float3 StandardLambertBlinnPhong(
    float3 normal,
    float3 viewDir,
    float3 lightDir,
    float3 ambientColor,
    float3 lightColor,
    float lightPower,
    float3 diffuseColor,
    float3 specularColor0,
    float3 specularColor1,
    float shininess0,
    float shininess1
) {
    normal = normalize(normal);
    viewDir = normalize(viewDir);
    lightDir = normalize(lightDir);
    float diffuseValue = saturate(dot(normal, lightDir));
    float specularValue0 = 0.0;
    float specularValue1 = 0.0;
    float3 reflectDir = normalize(reflect(-lightDir, normal));
    
    if (shininess0 > 0)
        specularValue0 = pow(saturate(dot(viewDir, reflectDir)), shininess0);
    if (shininess1 > 0)
        specularValue1 = pow(saturate(dot(viewDir, reflectDir)), shininess1);
    
    //float3 halfVector = normalize(lightDir + viewDir);
    //specularValue = pow(saturate(dot(normal, halfVector)), shininess);
    
    
    float3 light = lightColor * lightPower;
    float3 colorLinear =
        ambientColor +
        diffuseColor * diffuseValue * light +
        specularColor0 * specularValue0 * light +
        specularColor1 * specularValue1 * light;

    return colorLinear;
}
