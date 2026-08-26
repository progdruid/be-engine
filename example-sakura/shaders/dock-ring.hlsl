/*

@be-material: dock-ring-material {
    EmissiveColor: float3 = (1.0, 1.0, 1.0)
    Intensity: float = 2.0
    Thickness: float = 0.06
}

@be-shader dock-ring {
    topology triangle-list
    rasterizer none-solid
    blend disable
    depth less

    vertex VertexFunction(position)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main dock-ring-material

    target s0 Albedo_RGB float3
    target s1 WorldNormal_XYZ float4
    target s2 SurfaceData float4
    target s3 EmissiveRGB float3
}

*/


/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"
#include "objectMaterial.hlsl"

struct dock_ring_material {
    float3 EmissiveColor;
    float Intensity;
    float Thickness;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    dock_ring_material _GeometryMain;
};

struct VertexInput {
    float3 Position : POSITION;
};

struct PixelOutput {
    float3 Albedo_RGB : SV_Target0;
    float4 WorldNormal_XYZ : SV_Target1;
    float4 SurfaceData : SV_Target2;
    float3 EmissiveRGB : SV_Target3;
};

// endregion
/*========================================================*/


struct Interpolators {
    float4 Position : SV_POSITION;
    float2 Local    : TEXCOORD0;
};

Interpolators VertexFunction(VertexInput input) {
    float3 center = mul(float4(0.0, 0.0, 0.0, 1.0), _GeometryObject.Model).xyz;
    float radius  = length(mul(float4(1.0, 0.0, 0.0, 0.0), _GeometryObject.Model).xyz);

    float3 right    = normalize(mul(float4(1.0, 0.0, 0.0, 0.0), _Frame.CameraInverseProjectionView).xyz);
    float3 up       = normalize(mul(float4(0.0, 1.0, 0.0, 0.0), _Frame.CameraInverseProjectionView).xyz);

    float2 corner   = input.Position.xz;
    float3 worldPos = center + (corner.x * right + corner.y * up) * (2.0 * radius);

    Interpolators output;
    output.Position = mul(float4(worldPos, 1.0), _GeometryObject.ProjectionView);
    output.Local    = corner;
    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float d     = length(input.Local);
    float outer = 0.5;
    float inner = 0.5 - _GeometryMain.Thickness;
    if (d > outer || d < inner) discard;

    PixelOutput output;
    output.Albedo_RGB      = float3(0.0, 0.0, 0.0);
    output.WorldNormal_XYZ = float4(0.0, 0.0, 0.0, 0.0);
    output.SurfaceData     = float4(0.0, 0.0, 0.0, 0.0);
    output.EmissiveRGB     = _GeometryMain.EmissiveColor * _GeometryMain.Intensity;
    return output;
}
