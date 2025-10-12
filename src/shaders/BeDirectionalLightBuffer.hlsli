
cbuffer DirectionalLightBuffer: register(b1) {
    float3 _DirectionalLightVector;
    float3 _DirectionalLightColor;
    float _DirectionalLightPower;
};