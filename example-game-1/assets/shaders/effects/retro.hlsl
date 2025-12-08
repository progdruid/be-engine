// Retro low-res upsample post-processing shader
// Downsample via quantized UVs, apply posterization, upsample for PS1/CRT pixelated vibes

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

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

Texture2D inputTexture : register(t0);
SamplerState inputSampler : register(s0);

// Retro effect parameters
static const float PixelSize = 128.0f;          // Size of pixelated blocks (higher = more pixelated)
static const int PosterizeLevels = 4;           // Number of color levels (lower = crunchier)
static const bool EnablePosterization = true;   // Toggle color posterization

float3 Posterize(float3 color, int levels)
{
    // Quantize color to discrete levels for that PS1 palette look
    return floor(color * levels) / float(levels);
}

float3 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;

    // Quantize UVs to create pixel grid (nearest-neighbor like effect)
    // This creates the "downsampling" by snapping to grid positions
    float2 quantizedUV = floor(uv * PixelSize) / PixelSize;

    // Sample from the quantized UV position
    float3 color = inputTexture.Sample(inputSampler, quantizedUV).rgb;

    // Apply posterization for that crunchy palette look
    if (EnablePosterization)
    {
        color = Posterize(color, PosterizeLevels);
    }

    return color;
}