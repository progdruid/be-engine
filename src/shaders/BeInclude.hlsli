#pragma once

cbuffer UniformBuffer: register(b0) {
    row_major float4x4 ViewProjection;
    float3 CameraPosition;
    
    float3 AmbientColor;
    float3 DirectionalLightVector;
    float3 DirectionalLightColor;
};
