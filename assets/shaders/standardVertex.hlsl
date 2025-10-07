#include <BeInclude.hlsli>

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV    : TEXCOORD0;
};

struct VertexOutput {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV    : TEXCOORD0;

    float3 ViewDirection : TEXCOORD1;
};

VertexOutput main(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _Model);

    VertexOutput output;
    output.Position = mul(worldPosition, _ViewProjection);
    output.ViewDirection = _CameraPosition - worldPosition.xyz;
    output.Normal = normalize(mul(input.Normal, (float3x3)_Model));
    output.UV = input.UV;

    return output;
}