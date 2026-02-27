#pragma once
#ifdef __APPLE__

#include <cstdint>
#include <memory>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BeBuffers.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#else
typedef void* id;
#endif

class MetalContext;
class MetalSwapchain;
class MetalTexture;
class MetalPipeline;
class BeRenderPass;

class MetalRenderer {

    expose
    BeUniformData UniformData;

    hide
    uint32_t _width;
    uint32_t _height;
    void* _nativeView;

#ifdef __OBJC__
    id<MTLDevice> _device;
    id<MTLBuffer> _uniformBuffer;
#else
    id _device;
    id _uniformBuffer;
#endif

    std::shared_ptr<MetalContext> _context;
    std::shared_ptr<MetalSwapchain> _swapchain;
    std::shared_ptr<MetalTexture> _depthTexture;

    std::vector<BeRenderPass*> _passes;

    expose
    explicit MetalRenderer(uint32_t width, uint32_t height, void* nativeView);
    ~MetalRenderer();

    auto LaunchDevice() -> void;

    auto AddRenderPass(BeRenderPass* renderPass) -> void;
    auto ClearPasses() -> void;
    auto InitialisePasses() const -> void;
    auto Render() -> void;

    auto GetWidth() const -> uint32_t { return _width; }
    auto GetHeight() const -> uint32_t { return _height; }

    auto GetContext() const -> std::shared_ptr<MetalContext> { return _context; }
    auto GetSwapchain() const -> std::shared_ptr<MetalSwapchain> { return _swapchain; }

#ifdef __OBJC__
    auto GetDevice() const -> id<MTLDevice> { return _device; }
#endif
};

#endif
