
/*

@be-material: object-material-for-geometry-pass {
    Model: matrix
    ProjectionView: matrix
    ViewerPosition: float3 = (0, 0, 0)
}

*/

/*========================================================*/
// region @be-auto-boilerplate
struct object_material_for_geometry_pass {
    float4x4 Model;
    float4x4 ProjectionView;
    float3 ViewerPosition;
};

// endregion
/*========================================================*/
