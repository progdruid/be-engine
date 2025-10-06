#include <BeInclude.hlsli>

cbuffer ObjectBuffer: register(b1) { row_major float4x4 Model; };

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Color    : COLOR0;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 Color    : COLOR0;

    float3 ViewDirection : TEXCOORD1;
};

VertexOutput main(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), Model);

    VertexOutput output;
    output.Position = mul(worldPosition, _ViewProjection);
    output.ViewDirection = _CameraPosition - worldPosition.xyz;
    output.Normal = normalize(mul(input.Normal, (float3x3)Model));
    output.Color = input.Color;
    
    return output;
}