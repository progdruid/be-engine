#include "BeMaterial.h"
#include "BeMaterialImpl.h"
#include "BeRendererImpl.h"
#include "DxUtils.h"

#include "BeRenderer.h"

BeMaterial::~BeMaterial() = default;
BeMaterial::BeMaterial(BeMaterial&& other) noexcept = default;
BeMaterial& BeMaterial::operator=(BeMaterial&& other) noexcept = default;

auto BeMaterial::CreatePlatformBuffer(BeRenderer& renderer) -> void {
    _platformImpl = std::make_unique<BeMaterialImpl>();
    _platformImpl->isFrequentlyUsed = _isFrequentlyUsed;
    _platformImpl->context = renderer.GetPlatformImpl()->context;

    auto device = renderer.GetPlatformImpl()->device;

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    const uint32_t sizeInBytes = static_cast<uint32_t>(_bufferData.size() * sizeof(float));
    bufferDesc.ByteWidth = ((sizeInBytes + 15) / 16) * 16;
    if (_isFrequentlyUsed) {
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    } else {
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = _bufferData.data();

    DxUtils::Check << device->CreateBuffer(&bufferDesc, &data, _platformImpl->cbuffer.GetAddressOf());
}

auto BeMaterial::UpdatePlatformBuffer() -> bool {
    if (!_cbufferDirty) return false;
    if (!_platformImpl || !_platformImpl->cbuffer) return false;

    auto& ctx = _platformImpl->context;

    if (_platformImpl->isFrequentlyUsed) {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        DxUtils::Check << ctx->Map(_platformImpl->cbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, _bufferData.data(), _bufferData.size() * sizeof(float));
        ctx->Unmap(_platformImpl->cbuffer.Get(), 0);
    } else {
        ctx->UpdateSubresource(_platformImpl->cbuffer.Get(), 0, nullptr, _bufferData.data(), 0, 0);
    }

    _cbufferDirty = false;
    return true;
}
