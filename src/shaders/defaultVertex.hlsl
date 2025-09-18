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
    o.Position = float4(input.Position, 1.0f);
    o.Color = input.Color;
    return o;
}