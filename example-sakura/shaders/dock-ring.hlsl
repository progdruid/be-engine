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
    float3 Right    : TEXCOORD1;
    float3 Up       : TEXCOORD2;
    float3 Axis     : TEXCOORD3;
    float  Phase    : TEXCOORD4;
};

Interpolators VertexFunction(VertexInput input) {
    float3 center = mul(float4(0.0, 0.0, 0.0, 1.0), _GeometryObject.Model).xyz;
    float radius  = length(mul(float4(1.0, 0.0, 0.0, 0.0), _GeometryObject.Model).xyz);

    float3 right = normalize(mul(float4(1.0, 0.0, 0.0, 0.0), _Frame.CameraInverseProjectionView).xyz);
    float3 up    = normalize(mul(float4(0.0, 1.0, 0.0, 0.0), _Frame.CameraInverseProjectionView).xyz);

    float2 corner   = input.Position.xz;
    float3 worldPos = center + (corner.x * right + corner.y * up) * (2.0 * radius);

    float seed = center.x * 12.9898 + center.y * 78.233 + center.z * 37.719;
    float phi  = frac(sin(seed) * 43758.5453) * 6.2831853;
    float cosT = frac(sin(seed * 1.7 + 4.1) * 43758.5453) * 2.0 - 1.0;
    float sinT = sqrt(saturate(1.0 - cosT * cosT));

    Interpolators output;
    output.Position = mul(float4(worldPos, 1.0), _GeometryObject.ProjectionView);
    output.Local    = corner;
    output.Right    = right;
    output.Up       = up;
    output.Axis     = float3(sinT * cos(phi), cosT, sinT * sin(phi));
    output.Phase    = frac(sin(seed * 3.1 + 2.7) * 43758.5453) * 100.0;
    return output;
}

float Hash11(float p) {
    return frac(sin(p * 127.1) * 43758.5453);
}

PixelOutput PixelFunction(Interpolators input) {
    float d = length(input.Local);
    float3 dirWorld = d > 1e-4 ? (input.Right * input.Local.x + input.Up * input.Local.y) / d : input.Axis;

    float h = dot(dirWorld, input.Axis);

    float t     = _Frame.Time + input.Phase;
    float seed  = floor(t * 8.0);
    float burst = step(0.88, Hash11(seed));
    float row   = floor(h * 9.0 + t * 0.6);

    float shift     = (Hash11(row * 13.1 + seed) - 0.5) * 0.12;
    float rowActive = step(0.5, Hash11(row * 7.3 + seed * 1.7));
    d += shift * rowActive * burst;

    float dropout = burst * step(0.82, Hash11(row * 3.7 + seed * 2.3));

    float outer = 0.5;
    float inner = 0.5 - _GeometryMain.Thickness;
    if (d > outer || d < inner || dropout > 0.5) discard;

    PixelOutput output;
    output.Albedo_RGB      = float3(0.0, 0.0, 0.0);
    output.WorldNormal_XYZ = float4(0.0, 0.0, 0.0, 0.0);
    output.SurfaceData     = float4(0.0, 0.0, 0.0, 0.0);
    output.EmissiveRGB     = _GeometryMain.EmissiveColor * _GeometryMain.Intensity;
    return output;
}
