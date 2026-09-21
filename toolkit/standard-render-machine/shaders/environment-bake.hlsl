/*

@be-material: environment-bake-material {
    FaceIndex: float = 0
    Equirect: texture2d = black
    EquirectSampler: sampler = linear-clamp
}


@be-shader environment-bake {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 main environment-bake-material

    target s0 EnvFace float4
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/be-bindless-tables.hlsl"

struct environment_bake_material {
    float FaceIndex;
};

struct DrawRoot {
    environment_bake_material* Main;
    uint Equirect;
    uint EquirectSampler;
};
[[vk::push_constant]] DrawRoot Root;

property environment_bake_material* _Main { get { return Root.Main; } }
property Texture2D Equirect { get { return Tex2DTable[Root.Equirect]; } }
property SamplerState EquirectSampler { get { return SamplerTable[Root.EquirectSampler]; } }

struct PixelOutput {
    float4 EnvFace : SV_Target0;
};

// endregion
/*========================================================*/

#include "ibl-common.hlsli"
#include "core/fullscreen-vertex.hlsl"

float2 SampleSphericalMap(float3 dir) {
    float2 uv = float2(atan2(dir.z, dir.x) * INV_TWO_PI, -asin(dir.y) * INV_PI);
    uv += 0.5;
    return uv;
}

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float3 dir = DirectionForFace((int)_Main.FaceIndex, input.UV);
    float2 uv = SampleSphericalMap(dir);
    float3 color = Equirect.Sample(EquirectSampler, uv).rgb;

    PixelOutput output;
    output.EnvFace = float4(color, 1.0);
    return output;
}
