
Texture2D InputTexture : register(t0);
SamplerState InputSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    return InputTexture.Sample(InputSampler, input.UV);
}
