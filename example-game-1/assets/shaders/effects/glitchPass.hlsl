// Full-screen digital glitch post-processing effect
// Creates chromatic aberration, scan line artifacts, and displacement glitches

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

Texture2D colorTexture : register(t0);
SamplerState inputSampler : register(s0);

// Glitch parameters
static const float ChromaticAberration = 0.015f;    // RGB channel shift amount
static const float ScanLineIntensity = 0.3f;         // Scan line visibility
static const float BlockSize = 0.05f;                 // Size of glitch blocks
static const float DisplacementAmount = 0.02f;       // Max displacement offset
static const float ColorCorruptionAmount = 0.2f;     // Color shift intensity

// Pseudo-random noise function
float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

// Simplex-like noise for smoother displacement
float noise(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);

    float a = random(i);
    float b = random(i + float2(1.0, 0.0));
    float c = random(i + float2(0.0, 1.0));
    float d = random(i + float2(1.0, 1.0));

    float2 u = f * f * (3.0 - 2.0 * f);

    float ab = lerp(a, b, u.x);
    float cd = lerp(c, d, u.x);
    return lerp(ab, cd, u.y);
}

// Chromatic aberration: separate RGB channels with slight offsets
float3 ChromaticAberrationEffect(float2 uv, Texture2D tex, SamplerState samp)
{
    float3 color = float3(0.0, 0.0, 0.0);

    // Offset each channel slightly differently
    float2 redOffset = uv + float2(ChromaticAberration, 0.0);
    float2 greenOffset = uv;
    float2 blueOffset = uv - float2(ChromaticAberration, 0.0);

    color.r = tex.Sample(samp, redOffset).r;
    color.g = tex.Sample(samp, greenOffset).g;
    color.b = tex.Sample(samp, blueOffset).b;

    return color;
}

// Scan line effect: horizontal lines that flicker
float ScanLineEffect(float2 uv, float time)
{
    float scanLine = sin(uv.y * 500.0) * 0.5 + 0.5;
    scanLine = pow(scanLine, 3.0);  // Make lines thinner
    return 1.0 - (scanLine * ScanLineIntensity);
}

// Block displacement: glitchy horizontal offset in blocks
float2 BlockDisplacement(float2 uv, float time)
{
    // Create blocky regions
    float2 blockCoord = floor(uv / BlockSize);

    // Random displacement per block
    float displacement = (random(blockCoord) - 0.5) * DisplacementAmount;

    // Only apply occasionally (flicker effect)
    float flicker = step(0.7, random(blockCoord + time));

    return float2(displacement * flicker, 0.0);
}

// Color corruption: random color shifts
float3 ColorCorruption(float3 color, float2 uv, float time)
{
    float3 corruption = float3(
        random(uv + time) - 0.5,
        random(uv + time * 0.7) - 0.5,
        random(uv + time * 0.3) - 0.5
    );

    return color + corruption * ColorCorruptionAmount;
}

// RGB shift effect with displacement
float3 RGBShift(float2 uv, Texture2D tex, SamplerState samp)
{
    float offset = 0.01;

    float3 color = float3(0.0, 0.0, 0.0);
    color.r = tex.Sample(samp, uv + float2(offset, 0.0)).r;
    color.g = tex.Sample(samp, uv).g;
    color.b = tex.Sample(samp, uv - float2(offset, 0.0)).b;

    return color;
}

float3 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;

    // Get base color with chromatic aberration
    float3 color = ChromaticAberrationEffect(uv, colorTexture, inputSampler);

    // Apply block displacement
    float2 displacement = BlockDisplacement(uv, 0.0);  // Use _Time if available
    float3 displacedColor = colorTexture.Sample(inputSampler, uv + displacement).rgb;

    // Blend displacement with chromatic aberration
    color = lerp(color, displacedColor, 0.3);

    // Apply RGB channel shift
    float3 rgbShifted = RGBShift(uv, colorTexture, inputSampler);
    color = lerp(color, rgbShifted, 0.2);

    // Apply scan lines
    float scanEffect = ScanLineEffect(uv, 0.0);
    color *= scanEffect;

    // Apply color corruption
    color = ColorCorruption(color, uv, 0.0);

    // Clamp to valid range
    color = saturate(color);

    return float4(color, 1.0);
}
