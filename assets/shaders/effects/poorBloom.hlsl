// Poor man's bloom post-processing shader
// Single-pass fake bloom: threshold luma, minimal blur via offset taps, add back for highlight glow

/*
@be-shader-header
{
    "pixel": "main"
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

// Bloom parameters
static const float BloomThreshold = 0.5f;       // Luminance threshold for bloom (0.0 to 1.0)
static const float BloomIntensity = 0.8f;       // Bloom contribution strength
static const float BloomRadius = 2.0f;          // Blur radius in texels (1.0 to 4.0 recommended)

float Luminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float3 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;
    float2 screenSize = float2(1920.0f, 1080.0f);
    float2 texelSize = 1.0f / screenSize;

    // Sample original color
    float3 originalColor = inputTexture.Sample(inputSampler, uv).rgb;
    float originalLum = Luminance(originalColor);

    // Threshold: only bloom bright pixels
    float threshold = step(BloomThreshold, originalLum);
    float3 thresholdedColor = originalColor * threshold;

    // Minimal blur via offset tap sampling (poor man's bloom)
    // Sample at cardinal directions + diagonals
    float3 blurredColor = thresholdedColor;  // Start with center

    // Cardinal taps
    blurredColor += inputTexture.Sample(inputSampler, uv + float2(BloomRadius, 0) * texelSize).rgb * threshold;
    blurredColor += inputTexture.Sample(inputSampler, uv - float2(BloomRadius, 0) * texelSize).rgb * threshold;
    blurredColor += inputTexture.Sample(inputSampler, uv + float2(0, BloomRadius) * texelSize).rgb * threshold;
    blurredColor += inputTexture.Sample(inputSampler, uv - float2(0, BloomRadius) * texelSize).rgb * threshold;

    // Diagonal taps
    float diagonalRadius = BloomRadius * 0.707f;  // 45 degrees
    blurredColor += inputTexture.Sample(inputSampler, uv + float2(diagonalRadius, diagonalRadius) * texelSize).rgb * threshold;
    blurredColor += inputTexture.Sample(inputSampler, uv - float2(diagonalRadius, diagonalRadius) * texelSize).rgb * threshold;
    blurredColor += inputTexture.Sample(inputSampler, uv + float2(diagonalRadius, -diagonalRadius) * texelSize).rgb * threshold;
    blurredColor += inputTexture.Sample(inputSampler, uv - float2(diagonalRadius, -diagonalRadius) * texelSize).rgb * threshold;

    // Average the samples (9 taps total: 1 center + 4 cardinal + 4 diagonal)
    blurredColor /= 9.0f;

    // Blend bloom back onto original
    float3 finalColor = originalColor + (blurredColor * BloomIntensity);

    return finalColor;
}