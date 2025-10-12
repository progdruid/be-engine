
cbuffer PointLightBuffer: register(b1) {
    float3 _PointLightPosition;
    float _PointLightRadius;
    float3 _PointLightColor;
    float _PointLightPower;
};