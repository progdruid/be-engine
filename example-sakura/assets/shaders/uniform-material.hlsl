
/*

@be-material: uniform-material {
    CameraProjectionView: matrix
    CameraInverseProjectionView: matrix
    NearFarPlane: float4 = (0, 0, 0, 0)
    CameraPosition: float3 = (0, 0, 0)
    AmbientColor: float3 = (0, 0, 0)
    Time: float = 0
}

*/

/*========================================================*/
// region @be-auto-boilerplate
struct uniform_material {
    float4x4 CameraProjectionView;
    float4x4 CameraInverseProjectionView;
    float4 NearFarPlane;
    float3 CameraPosition;
    float3 AmbientColor;
    float Time;
};

// endregion
/*========================================================*/
