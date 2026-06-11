/*
@be-material: test-compute-material
[
    "ColorInput: texture2d = black",
    "Output: storage texture2d = storage-black",
]
@be-end

@be-shader: test-compute
{
    "compute": "CSMain",
    "materials": {
        "main": { "scheme": "test-compute-material", "slot": 0 }
    }
}
@be-end
*/

/*========================================================*/
// region @be-auto-boilerplate
Texture2D<float4>   ColorInput : register(t1, space0);
RWTexture2D<float4> Output     : register(u2, space0);

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
