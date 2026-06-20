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
