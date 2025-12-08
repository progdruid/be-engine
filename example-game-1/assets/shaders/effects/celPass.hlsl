
// Single-pass cel shader with world-position based edge detection
// Creates cartoon-like posterized effect with white edges from geometry discontinuities

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
Texture2D depthTexture : register(t1);
Texture2D normalTexture : register(t2);
SamplerState inputSampler : register(s0);

// Cel shader parameters
static const int ColorLevels = 6;           // Number of color bands (higher = more detail)
static const float PositionEdgeThreshold = 0.5f;    // World position discontinuity threshold
static const float NormalEdgeThreshold = 0.3f;     // Normal discontinuity threshold
static const float EdgeWidth = 1.0f;        // White edge intensity

float Luminance(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

float3 PosterizeColor(float3 color, int levels)
{
    // Quantize color to discrete levels
    return floor(color * levels) / levels;
}

float3 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;
    float2 texelSize = 1.0 / float2(1920, 1080);

    // Sample original pixel
    float3 originalColor = colorTexture.Sample(inputSampler, uv).rgb;
    float originalDepth = depthTexture.Sample(inputSampler, uv).r;
    float3 originalWorldPos = ReconstructWorldPosition(uv, originalDepth, _InverseProjectionView);
    float3 originalNormal = normalTexture.Sample(inputSampler, uv).rgb;

    // Edge detection: world position and normal discontinuities
    float maxPositionEdge = 0.0;
    float maxNormalEdge = 0.0;

    for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
        if (x == 0 && y == 0) continue;

        float2 offset = float2(x, y) * texelSize;
        float2 sampleUV = uv + offset;
        float sampledDepth = depthTexture.Sample(inputSampler, sampleUV).r;
        float3 sampledWorldPos = ReconstructWorldPosition(sampleUV, sampledDepth, _InverseProjectionView);
        float3 sampledNormal = normalTexture.Sample(inputSampler, sampleUV).rgb;

        // World position edge detection
        float3 positionDelta = sampledWorldPos - originalWorldPos;
        float positionEdge = length(positionDelta);
        maxPositionEdge = max(maxPositionEdge, positionEdge);

        // Normal edge detection (surface orientation difference)
        float normalDot = dot(sampledNormal, originalNormal);
        float normalEdge = 1.0 - normalDot;
        maxNormalEdge = max(maxNormalEdge, normalEdge);
    }}

    // Posterize the color
    float3 posterizedColor = PosterizeColor(originalColor, ColorLevels);

    // Detect edges from either world position OR normal discontinuities
    float positionIsEdge = step(PositionEdgeThreshold, maxPositionEdge);
    float normalIsEdge = step(NormalEdgeThreshold, maxNormalEdge);
    float isEdge = max(positionIsEdge, normalIsEdge);

    // Output black for edges, posterized color otherwise
    float3 finalColor = lerp(posterizedColor, float3(0.0, 0.0, 0.0), isEdge * EdgeWidth);

    return finalColor;
}