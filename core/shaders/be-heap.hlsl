#ifndef BE_HEAP_HLSL
#define BE_HEAP_HLSL

[[vk::binding(0, 0)]] Texture2D           Tex2DHeap[];
[[vk::binding(1, 0)]] Texture2DArray      Tex2DArrayHeap[];
[[vk::binding(2, 0)]] TextureCube         TexCubeHeap[];
[[vk::binding(3, 0)]] TextureCubeArray    TexCubeArrayHeap[];
[[vk::binding(4, 0)]] RWTexture2D<float4> RWTex2DHeap[];
[[vk::binding(5, 0)]] SamplerState        SamplerHeap[];

#endif
