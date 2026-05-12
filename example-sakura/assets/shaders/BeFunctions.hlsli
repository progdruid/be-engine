
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

float SmithGGXHelper (float nx, float k) {
    return nx / (nx * (1.0 - k) + k);
}

// orm: R=occlusion, G=roughness, B=metallic (matches GLTF metallicRoughness channel layout)
float3 StandardPBR (
    float3 normal,
    float3 viewDir,
    float3 lightDir,
    float3 light,
    float3 albedo,
    float3 orm
) {
    float metallic  = orm.z;
    float roughness = max(orm.y, 0.01);

    float3
    n = normalize(normal),
    v = normalize(viewDir),
    l = normalize(lightDir),
    h = normalize(l + v);

    float
    nl = saturate(dot(n, l)),
    nv = max(dot(n, v), 0.0001),
    nh = saturate(dot(n, h)),
    vh = saturate(dot(v, h));

    // Fresnel
    float one_minus_vh = 1.0 - vh;
    float3 f0 = lerp((0.04).xxx, albedo, metallic.xxx);
    float3 f = f0 + (1.0 - f0) * (one_minus_vh * one_minus_vh * one_minus_vh * one_minus_vh * one_minus_vh);

    // GGX Normal Distribution Function
    float
    a2 = roughness * roughness * roughness * roughness,
    temp = nh * nh * (a2 - 1.0) + 1.0,
    d = a2 / (PI * temp * temp);

    // Smith GGX
    float
    k = (roughness + 1.0) * (roughness + 1.0) * 0.125,
    g = SmithGGXHelper(nv, k) * SmithGGXHelper(nl, k);

    float3
    diffuse = ((1.0).xxx - f) * (1.0 - metallic) * (albedo / PI),
    specular = (f*d*g) / max(4.0 * nv * nl, 0.001),
    color = (diffuse + specular) * light * nl;
    return color;
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
