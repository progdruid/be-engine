#pragma once
#ifdef __APPLE__

#include <umbrellas/access-modifiers.hpp>

#include "../RhiSwapchain.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#else
typedef void* id;
#endif

class MetalSwapchain final : public RhiSwapchain {

    hide
#ifdef __OBJC__
    CAMetalLayer* _layer;
    id<MTLDevice> _device;
    id<CAMetalDrawable> _currentDrawable;
#else
    id _layer;
    id _device;
    id _currentDrawable;
#endif
    uint32_t _width;
    uint32_t _height;

    expose
#ifdef __OBJC__
    explicit MetalSwapchain(id<MTLDevice> device, void* nativeWindowLayer, uint32_t width, uint32_t height);
#else
    explicit MetalSwapchain(id device, void* nativeWindowLayer, uint32_t width, uint32_t height);
#endif
    ~MetalSwapchain() override;

    auto Present(uint32_t syncInterval) -> void override;
    auto Resize(uint32_t width, uint32_t height) -> void override;

#ifdef __OBJC__
    auto GetCurrentDrawable() -> id<CAMetalDrawable>;
    auto GetLayer() const -> CAMetalLayer* { return _layer; }
#endif

    auto GetWidth() const -> uint32_t { return _width; }
    auto GetHeight() const -> uint32_t { return _height; }
};

#endif
