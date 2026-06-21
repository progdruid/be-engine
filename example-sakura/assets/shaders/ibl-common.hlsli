static const float PI          = 3.14159265359;
static const float INV_PI      = 0.31830988618;
static const float INV_TWO_PI  = 0.15915494309;

float3 DirectionForFace(int face, float2 uv) {
    float u = 2.0 * uv.x - 1.0;
    float v = 2.0 * uv.y - 1.0;

    float3 dir;
    if      (face == 0) dir = float3( 1.0, -v, -u);
    else if (face == 1) dir = float3(-1.0, -v,  u);
    else if (face == 2) dir = float3(   u, 1.0,  v);
    else if (face == 3) dir = float3(   u,-1.0, -v);
    else if (face == 4) dir = float3(   u,  -v, 1.0);
    else                dir = float3(  -u,  -v,-1.0);

    return normalize(dir);
}

float RadicalInverseVdC(uint bits) {
    return float(reversebits(bits)) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint count) {
    return float2(float(i) / float(count), RadicalInverseVdC(i));
}

float3 ImportanceSampleGGX(float2 xi, float3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}
