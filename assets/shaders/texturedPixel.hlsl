Texture2D texture0 : register(t0);
SamplerState samp : register(s0);

struct PixelInput {
    float4 Position : SV_Position;
    float2 UV    : TEXCOORD0;
};

float4 main(PixelInput input) : SV_Target {
    float4 color = texture0.Sample(samp, input.UV);
    if (color.a < 0.5) discard;
    return color;
};