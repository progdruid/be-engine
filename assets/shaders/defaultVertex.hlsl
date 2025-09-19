cbuffer UniformBuffer: register(b0) { row_major float4x4 ViewProjection; };
cbuffer ObjectBuffer: register(b1) { row_major float4x4 Model; };

struct VertexInput {
    float3 Position : POSITION;
    float3 Color    : COLOR0;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 Color    : COLOR0;
};

VertexOutput main(VertexInput input) {
    VertexOutput o;
    o.Position = mul(float4(input.Position, 1.0), Model);
    o.Position = mul(o.Position, ViewProjection);
    o.Color = input.Color;
    return o;
}