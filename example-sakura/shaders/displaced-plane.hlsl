
/*

@be-material: displaced-plane-material-for-geometry-pass {
    DeepColor: float3 = #051433
    PeakColor: float3 = #73C7F2
    Amplitude: float = 1.5
    Frequency: float = 0.4
    Speed: float = 1.0
}


@be-shader displaced-plane {
    topology triangle-list

    vertex VertexFunction(position)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main displaced-plane-material-for-geometry-pass

    target s0 Albedo_RGB float3
    target s1 WorldNormal_XYZ float4
    target s2 ORM_RGB float4
    target s3 EmissiveRGB float3
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/uniform-material.hlsl"
#include "core/objectMaterial.hlsl"

struct displaced_plane_material_for_geometry_pass {
    float3 DeepColor;
    float3 PeakColor;
    float Amplitude;
    float Frequency;
    float Speed;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    displaced_plane_material_for_geometry_pass _GeometryMain;
};

struct VertexInput {
    float3 Position : POSITION;
};

struct PixelOutput {
    float3 Albedo_RGB : SV_Target0;
    float4 WorldNormal_XYZ : SV_Target1;
    float4 ORM_RGB : SV_Target2;
    float3 EmissiveRGB : SV_Target3;
};

// endregion
/*========================================================*/

struct Interpolators {
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
    float Height : TEXCOORD1;
};

float WaveHeight(float2 p, float t) {
    float h = 0.0;
    h += sin(p.x * 1.0 + t) * 0.50;
    h += sin(p.y * 1.3 - t * 0.8) * 0.40;
    h += sin((p.x + p.y) * 0.7 + t * 1.3) * 0.30;
    h += sin((p.x - p.y) * 1.7 - t * 0.6) * 0.15;
    return h;
}

Interpolators VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _GeometryObject.Model);

    float t = _Frame.Time * _GeometryMain.Speed;
    float2 wavePos = worldPosition.xz * _GeometryMain.Frequency;
    float height = WaveHeight(wavePos, t) * _GeometryMain.Amplitude;
    worldPosition.y += height;

    Interpolators output;
    output.Position = mul(worldPosition, _GeometryObject.ProjectionView);
    output.WorldPosition = worldPosition.xyz;
    output.Height = height;
    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float3 faceNormal = normalize(cross(ddx(input.WorldPosition), ddy(input.WorldPosition)));

    float gradient = saturate(input.Height * 0.5 + 0.5);
    float3 albedo = lerp(_GeometryMain.DeepColor, _GeometryMain.PeakColor, gradient);

    PixelOutput output;
    output.Albedo_RGB          = albedo;
    output.WorldNormal_XYZ.xyz = faceNormal;
    output.WorldNormal_XYZ.w   = 0.0;
    output.ORM_RGB             = float4(1.0, 0.65, 0.0, 0.0);
    output.EmissiveRGB         = (0.0).rrr;
    return output;
}
