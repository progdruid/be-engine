struct VSInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float4 color    : COLOR0;
};

struct VSOutput {
    float4 sv_position                  : SV_Position;
    [[vk::location(0)]] float4 color    : COLOR0;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.sv_position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}

float4 PSMain(VSOutput input) : SV_Target0 {
    return input.color;
}
