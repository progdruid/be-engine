#include "BeMaterial.h"
#include "BeMaterialImpl.h"
#include "BeRendererImpl.h"
#include "MetalUtils.h"

#include "BeRenderer.h"

#import <Metal/Metal.h>

BeMaterial::~BeMaterial() = default;
BeMaterial::BeMaterial(BeMaterial&& other) noexcept = default;
BeMaterial& BeMaterial::operator=(BeMaterial&& other) noexcept = default;

auto BeMaterial::CreatePlatformBuffer(BeRenderer& renderer) -> void {
    _platformImpl = std::make_unique<BeMaterialImpl>();
    _platformImpl->isFrequentlyUsed = _isFrequentlyUsed;

    auto device = renderer.GetPlatformImpl()->device;

    const uint32_t sizeInBytes = static_cast<uint32_t>(_bufferData.size() * sizeof(float));
    const uint32_t alignedSize = ((sizeInBytes + 15) / 16) * 16;

    _platformImpl->cbuffer = [device newBufferWithBytes:_bufferData.data()
                                                 length:alignedSize
                                                options:MTLResourceStorageModeShared];
}

auto BeMaterial::UpdatePlatformBuffer() -> bool {
    if (!_cbufferDirty) return false;
    if (!_platformImpl || !_platformImpl->cbuffer) return false;

    memcpy([_platformImpl->cbuffer contents], _bufferData.data(), _bufferData.size() * sizeof(float));

    _cbufferDirty = false;
    return true;
}
