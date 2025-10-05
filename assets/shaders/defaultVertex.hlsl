#include <BeInclude.hlsli>

cbuffer ObjectBuffer: register(b1) { row_major float4x4 Model; };

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Color    : COLOR0;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 WorldPosition : POSITION1;
    float3 Normal : NORMAL;
    float3 Color    : COLOR0;
};

VertexOutput main(VertexInput input) {
    VertexOutput o;
    o.Position = mul(float4(input.Position, 1.0), Model);
    o.WorldPosition = o.Position.xyz;
    o.Position = mul(o.Position, _ViewProjection);
    o.Normal = mul(float4(input.Normal, 1.f), (float3x3)Model);
    o.Color = input.Color;
    return o;
}