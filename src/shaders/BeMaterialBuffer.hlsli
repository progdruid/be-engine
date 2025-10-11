
cbuffer MaterialBuffer: register(b1) {
    row_major float4x4 _Model;
    
    float3 _DiffuseColor;
    float3 _SpecularColor0;
    float _Shininess0;
    float3 _SpecularColor1;
    float _Shininess1;
};

SamplerState DefaultSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
Texture2D Specular : register(t1);