// Chromatic aberration post-processing shader
// Simulates lens chromatic aberration by separating RGB channels with spatial offsets

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

// Chromatic aberration parameters
static const float AberrationStrength = 0.01f;  // How strong the effect is (0.005f to 0.02f works well)
static const float RedShift = 1.0f;             // Red channel offset multiplier
static const float GreenShift = 0.0f;           // Green channel offset multiplier (usually center)
static const float BlueShift = -1.0f;           // Blue channel offset multiplier

float3 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;

    // Center the UV coordinates around (0.5, 0.5) for aberration calculation
    float2 centerUV = uv - 0.5f;

    // Sample each color channel with different offsets
    // The offset is based on the direction from screen center
    float2 aberrationDir = normalize(centerUV + 0.0001f); // Add small value to avoid division by zero
    float aberrationDistance = length(centerUV);

    // Red channel - offset outward
    float2 redUV = uv + aberrationDir * AberrationStrength * RedShift;
    float3 redChannel = inputTexture.Sample(inputSampler, redUV).rgb;

    // Green channel - no offset (center)
    float2 greenUV = uv + aberrationDir * AberrationStrength * GreenShift;
    float3 greenChannel = inputTexture.Sample(inputSampler, greenUV).rgb;

    // Blue channel - offset inward
    float2 blueUV = uv + aberrationDir * AberrationStrength * BlueShift;
    float3 blueChannel = inputTexture.Sample(inputSampler, blueUV).rgb;

    // Combine the channels
    float3 finalColor = float3(
        redChannel.r,
        greenChannel.g,
        blueChannel.b
    );

    return finalColor;
}