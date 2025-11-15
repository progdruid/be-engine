/*
@be-shader-header
{
    "vertex": "VertexFunction",
    "pixel": "PixelFunction",
    "vertexLayout": ["position"]
}
@be-shader-header-end
*/

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

VertexOutput VertexFunction (VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _Model);
    VertexOutput output;
    output.Position = mul(worldPosition, _ProjectionView);
    output.WorldPosition = worldPosition.xyz;
    return output;
}

float4 PixelFunction(VertexOutput input) : SV_TARGET {
    float distance = length(input.WorldPosition - _LightPosition);
    return float4(distance, 0.0, 0.0, 1.0);
}
