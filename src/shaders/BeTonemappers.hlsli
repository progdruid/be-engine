
float3 Tonemap_Reinhard(float3 x) {
    return x / (1.0 + x);
}

float3 Tonemap_ReinhardWhite(float3 x, float white) { // white ~ 2–4 
    float3 num = x * (1.0 + x / (white * white));
    return num / (1.0 + x);
}

float3 Tonemap_Exponential(float3 x, float a) // a ~ 1.0
{
    return 1.0 - exp(-a * x);
}

// Parameters from J. Hable's writeups; tweakable.
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
    // Slightly modified fast filmic curve
    float3 X = max(0.0, x - 0.004);
    return (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
}

// Stephen Hill's ACES fitted (linear in/out; sRGB transfer after)
// sRGB -> ACEScg-ish -> RRT/ODT fit -> sRGB-ish
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

