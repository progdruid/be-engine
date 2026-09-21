/*

@be-material: mip-downsample-material {
    TexelSize: float2 = (0.001, 0.001)
    SourceMip: texture2d = white
    SourceSampler: sampler = linear-clamp
}

@be-shader mip-downsample {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PixelFunction

    bind s0 main mip-downsample-material

    target s0 MipOutput float4
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "core/be-bindless-tables.hlsl"

struct mip_downsample_material {
    float2 TexelSize;
};

struct DrawRoot {
    mip_downsample_material* Main;
    uint SourceMip;
    uint SourceSampler;
};
[[vk::push_constant]] DrawRoot Root;

property mip_downsample_material* _Main { get { return Root.Main; } }
property Texture2D SourceMip { get { return Tex2DTable[Root.SourceMip]; } }
property SamplerState SourceSampler { get { return SamplerTable[Root.SourceSampler]; } }

struct PixelOutput {
    float4 MipOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "core/fullscreen-vertex.hlsl"

PixelOutput PixelFunction(FullscreenVSOutput input) {
    float2 t = _Main.TexelSize;
    float2 uv = input.UV;

    float4 a = SourceMip.SampleLevel(SourceSampler, uv + t * float2(-0.5, -0.5), 0);
    float4 b = SourceMip.SampleLevel(SourceSampler, uv + t * float2( 0.5, -0.5), 0);
    float4 c = SourceMip.SampleLevel(SourceSampler, uv + t * float2(-0.5,  0.5), 0);
    float4 d = SourceMip.SampleLevel(SourceSampler, uv + t * float2( 0.5,  0.5), 0);

    PixelOutput output;
    output.MipOutput = (a + b + c + d) * 0.25;
    return output;
}
