// Edge glow post-processing shader
// Sobel edge detection on luminance + colored glow where edges are strong

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

// Edge glow parameters
static const float EdgeThreshold = 0.1f;        // Minimum edge strength to glow (0.0 to 1.0)
static const float GlowIntensity = 1.5f;        // How bright the glow is
static const float GlowSoftness = 2.0f;         // Remap curve for soft falloff (higher = softer)
static const float3 GlowColor = float3(0.3f, 0.6f, 1.0f);  // Color tint for glow (blue)

float Luminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float3 SampleColor(float2 uv)
{
    return inputTexture.Sample(inputSampler, uv).rgb;
}

// Sobel edge detection on luminance
float SobelEdgeDetection(float2 uv, float texelSize)
{
    // Sobel X and Y kernels
    // Sample 3x3 neighborhood
    float tl = Luminance(SampleColor(uv + float2(-texelSize, texelSize)));      // Top-left
    float t  = Luminance(SampleColor(uv + float2(0, texelSize)));               // Top
    float tr = Luminance(SampleColor(uv + float2(texelSize, texelSize)));       // Top-right

    float l  = Luminance(SampleColor(uv + float2(-texelSize, 0)));              // Left
    float r  = Luminance(SampleColor(uv + float2(texelSize, 0)));               // Right

    float bl = Luminance(SampleColor(uv + float2(-texelSize, -texelSize)));     // Bottom-left
    float b  = Luminance(SampleColor(uv + float2(0, -texelSize)));              // Bottom
    float br = Luminance(SampleColor(uv + float2(texelSize, -texelSize)));      // Bottom-right

    // Sobel X kernel
    float sobelX = (tl * -1.0f + tr * 1.0f +
                    l * -2.0f + r * 2.0f +
                    bl * -1.0f + br * 1.0f);

    // Sobel Y kernel
    float sobelY = (tl * -1.0f + t * -2.0f + tr * -1.0f +
                    bl * 1.0f + b * 2.0f + br * 1.0f);

    // Magnitude of gradient
    float edgeMagnitude = length(float2(sobelX, sobelY));

    return edgeMagnitude;
}

float3 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;
    float2 screenSize = float2(1920.0f, 1080.0f);
    float texelSize = 1.0f / screenSize.x;  // Using width for texel size

    // Sample original color
    float3 originalColor = SampleColor(uv);

    // Detect edges via Sobel
    float edgeMagnitude = SobelEdgeDetection(uv, texelSize);

    // Remap edge magnitude for softness control
    // Higher GlowSoftness = softer falloff
    float remappedEdge = pow(saturate(edgeMagnitude / GlowSoftness), 1.0f / GlowSoftness);

    // Only apply glow where edges exceed threshold
    float glowMask = step(EdgeThreshold, remappedEdge);

    // Calculate glow contribution
    float glow = remappedEdge * glowMask * GlowIntensity;

    // Blend glow color with original
    float3 finalColor = originalColor + (GlowColor * glow);

    return finalColor;
}