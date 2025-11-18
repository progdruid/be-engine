
cbuffer UniformBuffer: register(b0) {
    row_major float4x4 _ProjectionView;
    row_major float4x4 _InverseProjectionView;
    // x = near, y = far, z = 1/near, w = 1/far
    float4 _NearFarPlane;
    float3 _CameraPosition;
    
    float3 _AmbientColor;

    float __pad;
    
    float _Time;
};
