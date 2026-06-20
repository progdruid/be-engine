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
struct environment_bake_material {
    float FaceIndex;
};

cbuffer CBuffer_0 : register(b0, space0) {
    environment_bake_material _Main;
};
SamplerState EquirectSampler : register(s1, space0);
Texture2D Equirect : register(t2, space0);

struct PixelOutput {
    float4 EnvFace : SV_Target0;
};

// endregion
/*========================================================*/

#include "ibl-common.hlsli"
#include "fullscreen-vertex.hlsl"

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
