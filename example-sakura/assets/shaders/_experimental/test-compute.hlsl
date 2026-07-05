/*
@be-material: test-compute-material {
    ColorInput: texture2d = black
    Output: storage texture2d = storage-black
}

@be-shader test-compute {
    compute CSMain

    bind s0 main test-compute-material
}
*/

/*========================================================*/
// region @be-auto-boilerplate
Texture2D ColorInput : register(t1, space0);
RWTexture2D<float4> Output : register(u2, space0);

// endregion
/*========================================================*/

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    Output.GetDimensions(width, height);
    if (id.x >= width || id.y >= height) {
        return;
    }

    float3 color = ColorInput.Load(int3(id.xy, 0)).rgb;
    float luma = dot(color, float3(0.299, 0.587, 0.114));
    Output[id.xy] = float4(luma, luma, luma, 1.0);
}
