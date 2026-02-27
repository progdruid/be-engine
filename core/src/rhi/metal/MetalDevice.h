#pragma once
#ifdef __APPLE__

#include <memory>
#include <umbrellas/access-modifiers.hpp>

#include "../RhiDevice.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

class MetalDevice final : public RhiDevice {

    hide
#ifdef __OBJC__
    id<MTLDevice> _device;
#else
    id _device;
#endif

    expose
    explicit MetalDevice();
    ~MetalDevice() override;

    auto CreateBuffer(const RhiBufferDesc& desc, const void* initialData) -> std::shared_ptr<RhiBuffer> override;
    auto CreateSampler(const RhiSamplerDesc& desc) -> std::shared_ptr<RhiSampler> override;

    auto GetBackendName() const -> const char* override { return "Metal"; }

#ifdef __OBJC__
    auto GetNative() const -> id<MTLDevice> { return _device; }
#else
    auto GetNativeRaw() const -> void* { return _device; }
#endif
};

#endif
