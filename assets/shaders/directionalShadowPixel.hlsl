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
    // For orthographic projection, linearized depth is simply the z coordinate
    // normalized to [0, 1] range
    float linearDepth = input.Position.z;
    return float4(linearDepth, 0.0, 0.0, 1.0);
}
