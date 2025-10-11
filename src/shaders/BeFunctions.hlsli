
#define PI 3.14159265359

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

float3 ReconstructWorldPosition(float2 uv, float depth01, float4x4 invProjectionView)
{
    float4 clipSpacePosition;
    uv.y = 1.0 - uv.y; // Flip Y for UV coordinates 
    clipSpacePosition.xy = uv * 2.0 - 1.0;
    clipSpacePosition.z = depth01;
    clipSpacePosition.w = 1.0;

    float4 worldSpacePosition = mul(clipSpacePosition, invProjectionView);
    worldSpacePosition /= worldSpacePosition.w;

    return worldSpacePosition.xyz;
}

float LinearizeDepth(float depth01, float nearZ, float farZ)
{
    float ndc = depth01 * 2.0 - 1.0;
    return (2.0 * nearZ * farZ) / (farZ + nearZ - ndc * (farZ - nearZ));
}
