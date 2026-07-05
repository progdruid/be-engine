
float3 Tonemap_Reinhard(float3 x) {
    return x / (1.0 + x);
}

float3 LinearToSrgb(float3 c) {
    return pow(saturate(c), 1.0 / 2.2);
}

float3 ApplyContrast(float3 color, float contrast) {
    const float pivot = 0.18;
    return pow(max(color, 0.0) / pivot, contrast) * pivot;
}

float3 Tonemap_ReinhardWhite(float3 x, float white) { // white ~ 2–4 
    float3 num = x * (1.0 + x / (white * white));
    return num / (1.0 + x);
}

float3 Tonemap_Exponential(float3 x, float a) // a ~ 1.0
{
    return 1.0 - exp(-a * x);
}

float3 HableCurve(float3 x, float A, float B, float C, float D, float E, float F)
{
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 Tonemap_HableU2(float3 x)
{
    const float A = 0.22;
    const float B = 0.30;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.01;
    const float F = 0.30;
    const float W = 11.2; // white point
    float3 whiteScale = 1.0 / HableCurve(W, A, B, C, D, E, F);
    float3 curr = HableCurve(x, A, B, C, D, E, F);
    return curr * whiteScale;
}

float3 Tonemap_HejlBurgessDawson(float3 x)
{
    float3 X = max(0.0, x - 0.004);
    return (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
}

float3 Tonemap_ACESFitted(float3 x)
{
    const float3x3 ACESInputMat = {
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    };

    const float3x3 ACESOutputMat = {
        1.60475,-0.53108,-0.07367,
       -0.10208, 1.10813,-0.00605,
       -0.00327,-0.07276, 1.07602
    };

    float3 v = mul(ACESInputMat, x);

    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    v = (v * (a * v + b)) / (v * (c * v + d) + e); // RRT+ODT fit
    v = mul(ACESOutputMat, v);
    return saturate(v);
}

float3 Tonemap_ACES_Knarkowicz(float3 x)
{
    const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 AgXDefaultContrastApprox(float3 x)
{
    float3 x2 = x * x;
    float3 x4 = x2 * x2;
    return 15.5 * x4 * x2
         - 40.14 * x4 * x
         + 31.96 * x4
         - 6.868 * x2 * x
         + 0.4298 * x2
         + 0.1191 * x
         - 0.00232;
}

float3 AgXLookPunchy(float3 color)
{
    const float3 lw = float3(0.2126, 0.7152, 0.0722);
    float luma = dot(color, lw);
    const float3 power = float3(1.35, 1.35, 1.35);
    const float sat = 1.4;
    color = pow(color, power);
    return luma + sat * (color - luma);
}

float3 Tonemap_AgX(float3 color)
{
    const float3x3 agxMat = {
        0.842479062253094, 0.0423282422610123, 0.0423756549057051,
        0.0784335999999992, 0.878468636469772, 0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104
    };
    const float3x3 agxMatInv = {
        1.19687900512017, -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433, 1.15107367264116
    };
    const float minEv = -12.47393;
    const float maxEv = 4.026069;

    color = mul(color, agxMat);
    color = clamp(log2(color), minEv, maxEv);
    color = (color - minEv) / (maxEv - minEv);
    color = AgXDefaultContrastApprox(color);
    color = AgXLookPunchy(color);
    color = mul(color, agxMatInv);
    return saturate(color);
}

