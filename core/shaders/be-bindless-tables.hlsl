#ifndef BE_BINDLESS_TABLES_HLSL
#define BE_BINDLESS_TABLES_HLSL

[[vk::binding(0, 0)]] Texture2D           Tex2DTable[];
[[vk::binding(0, 0)]] Texture2DArray      Tex2DArrayTable[];
[[vk::binding(0, 0)]] TextureCube         TexCubeTable[];
[[vk::binding(0, 0)]] TextureCubeArray    TexCubeArrayTable[];
[[vk::binding(1, 0)]] RWTexture2D<float4> RWTex2DTable[];
[[vk::binding(2, 0)]] SamplerState        SamplerTable[];

#endif
