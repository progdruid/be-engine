cbuffer ShadowBuffer : register(b1) {
    row_major float4x4 _ProjectionView;
    row_major float4x4 _Model;
    float3 _LightPosition;
};

struct PixelInput {
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
};

float4 main(PixelInput input) : SV_TARGET {
    float distance = length(input.WorldPosition - _LightPosition);
    return float4(distance, 0.0, 0.0, 1.0);
}
