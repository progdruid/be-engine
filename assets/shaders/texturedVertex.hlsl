#include <BeInclude.hlsli>

cbuffer UniformBuffer: register(b0) { row_major float4x4 ViewProjection; };
cbuffer ObjectBuffer: register(b1) { row_major float4x4 Model; };

struct VertexInput {
    float3 Position : POSITION;
    float2 UV    : TEXCOORD0;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    float2 UV    : TEXCQOORD0;
};

VertexOutput main(VertexInput input) {
    Hello hello; // To test BeInclude.hlsli inclusion
    VertexOutput o;
    o.Position = mul(float4(input.Position, 1.0), Model);
    o.Position = mul(o.Position, ViewProjection);
    o.UV = input.UV;
    return o;
}