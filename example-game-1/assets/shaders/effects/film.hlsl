// Film grain + dithering post-processing shader
// Hash-based per-pixel grain in log space + ordered dithering for smooth banding reduction

/*
@be-shader-header
{
    "pixel": "main",
    "targets": {
        "HDRTarget": 0
    }
}
@be-shader-header-end
*/

#include <BeUniformBuffer.hlsli>
#include <BeFunctions.hlsli>

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

Texture2D inputTexture : register(t0);
SamplerState inputSampler : register(s0);

// Film grain and dithering parameters
static const float GrainStrength = 0.3f;        // Film grain intensity (0.0 to 0.3 recommended)
static const float GrainScale = 1.0f;           // Frequency of grain pattern
static const bool UseLogSpace = true;           // Apply grain in log space (more natural on dark areas)
static const bool EnableDithering = true;       // Toggle ordered dithering
static const float DitheringStrength = 3.8f;    // Dithering intensity

// Hash function for pseudo-random grain (based on screen position)
float hash(float2 p)
{
    float3 p3 = frac(float3(p.x, p.y, p.x) * 0.13f);
    p3 += dot(p3, p3.yzx + 3.333f);
    return frac((p3.x + p3.y) * p3.z);
}

// Bayer matrix for ordered dithering (4x4)
float BayerMatrix4x4(float2 pixelPos)
{
    // 4x4 Bayer matrix normalized to 0-1
    int x = int(pixelPos.x) % 4;
    int y = int(pixelPos.y) % 4;

    if (y == 0)
    {
        if (x == 0) return 0.0f / 16.0f;
        if (x == 1) return 8.0f / 16.0f;
        if (x == 2) return 2.0f / 16.0f;
        return 10.0f / 16.0f;
    }
    else if (y == 1)
    {
        if (x == 0) return 12.0f / 16.0f;
        if (x == 1) return 4.0f / 16.0f;
        if (x == 2) return 14.0f / 16.0f;
        return 6.0f / 16.0f;
    }
    else if (y == 2)
    {
        if (x == 0) return 3.0f / 16.0f;
        if (x == 1) return 11.0f / 16.0f;
        if (x == 2) return 1.0f / 16.0f;
        return 9.0f / 16.0f;
    }
    else // y == 3
    {
        if (x == 0) return 15.0f / 16.0f;
        if (x == 1) return 7.0f / 16.0f;
        if (x == 2) return 13.0f / 16.0f;
        return 5.0f / 16.0f;
    }
}

float3 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;
    float2 pixelPos = uv * float2(1920.0f, 1080.0f); // Screen resolution

    // Sample input color
    float3 color = inputTexture.Sample(inputSampler, uv).rgb;

    // Apply film grain
    if (GrainStrength > 0.0f)
    {
        // Generate hash-based grain
        float grain = hash(pixelPos * GrainScale) * 2.0f - 1.0f; // Range: -1 to 1

        if (UseLogSpace)
        {
            // Apply grain in log space for more natural appearance on dark areas
            // Grain affects darker areas more, lighter areas less
            float luminance = dot(color, float3(0.299f, 0.587f, 0.114f));
            float logLum = log(luminance + 0.001f); // Avoid log(0)
            logLum += grain * GrainStrength;
            color *= exp(logLum - log(luminance + 0.001f));
        }
        else
        {
            // Apply grain in linear space
            color += grain * GrainStrength;
        }
    }

    // Apply ordered dithering to reduce banding
    if (EnableDithering)
    {
        float dither = BayerMatrix4x4(pixelPos);
        // Offset dither value around 0 (range: -0.5 to 0.5)
        dither = (dither - 0.5f) * DitheringStrength / 255.0f;
        color += dither;
    }

    // Clamp to valid range
    color = clamp(color, 0.0f, 1.0f);

    return color;
}