
/*

@be-material: object-material-for-geometry-pass {
    Model: matrix
    ProjectionView: matrix
    ViewerPosition: float3 = (0, 0, 0)
}

*/


struct object_material_for_geometry_pass {
    row_major float4x4 Model;
    row_major float4x4 ProjectionView;
    float3 ViewerPosition;
};