#pragma once

#include <array>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct BeRendererImpl;

struct BePipelineImpl {
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Device> device;
    BeRendererImpl* rendererImpl = nullptr;

    std::array<uint32_t, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> vertexResCache;
    std::array<uint32_t, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> tessResCache;
    std::array<uint32_t, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> pixelResCache;

    std::array<uint32_t, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> vertexCBufferIDCache;
    std::array<uint32_t, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> tessCBufferIDCache;
    std::array<uint32_t, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> pixelCBufferIDCache;

    std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> vertexSamplerCache;
    std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> tessSamplerCache;
    std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> pixelSamplerCache;
};
