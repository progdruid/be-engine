
#define PI 3.14159265359

cbuffer UniformBuffer: register(b0) {
    row_major float4x4 _ViewProjection;
    float3 _CameraPosition;
    
    float3 _AmbientColor;
    float3 _DirectionalLightVector;
    float3 _DirectionalLightColor;
    float _DirectionalLightPower;
};
