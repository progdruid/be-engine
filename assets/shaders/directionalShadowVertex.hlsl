cbuffer ShadowBuffer : register(b1) {
    row_major float4x4 _ProjectionView;
    row_major float4x4 _Model;
    float3 _LightPosition;
};

struct VertexInput {
    float3 Position : POSITION;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
};

VertexOutput main(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _Model);
    VertexOutput output;
    output.Position = mul(worldPosition, _ProjectionView);
    output.WorldPosition = worldPosition.xyz;
    return output;
}
