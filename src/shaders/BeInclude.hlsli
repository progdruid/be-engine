#pragma once

cbuffer UniformBuffer: register(b0) {
    row_major float4x4 ViewProjection;
    float4 CameraPosition;
    
    float3 AmbientColor;
    float AmbientIntensity;
    float3 DirectionalLightColor;
    float3 DirectionalLightVector;
};
