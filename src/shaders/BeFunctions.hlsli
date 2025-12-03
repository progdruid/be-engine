
#define PI 3.14159265359

float3 StandardLambertBlinnPhong(
    float3 normal,
    float3 viewDir,
    float3 lightDir,
    //float3 ambientColor,
    float3 lightColor,
    float lightPower,
    float3 diffuseColor,
    float3 specularColor,
    float shininess
) {
    normal = normalize(normal);
    viewDir = normalize(viewDir);
    lightDir = normalize(lightDir);
    float diffuseValue = saturate(dot(normal, lightDir));
    float specularValue = 0.0;
    float3 reflectDir = normalize(reflect(-lightDir, normal));
    
    if (shininess > 0.00001)
        specularValue = pow(saturate(dot(viewDir, reflectDir)), shininess * 2048.f);
    
    //float3 halfVector = normalize(lightDir + viewDir);
    //specularValue = pow(saturate(dot(normal, halfVector)), shininess);
    
    
    float3 light = lightColor * lightPower;
    float3 colorLinear =
        //ambientColor +
        diffuseColor * diffuseValue * light +
        specularColor * specularValue * light;

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
