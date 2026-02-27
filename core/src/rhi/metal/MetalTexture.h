#pragma once
#ifdef __APPLE__

#include <cstdint>
#include <memory>
#include <string>
#include <umbrellas/access-modifiers.hpp>

#include "../RhiTypes.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

class MetalTexture {

    expose struct Descriptor {
        std::string Name;
        bool IsCubemap = false;
        RhiFormat Format = RhiFormat::R8G8B8A8_UNORM;
        RhiBindFlags BindFlags = RhiBindFlags::ShaderResource;
        uint32_t Mips = 1;
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint8_t* Data = nullptr;
    };

    hide
#ifdef __OBJC__
    id<MTLTexture> _texture;
    id<MTLTexture> _depthTexture;
#else
    id _texture;
    id _depthTexture;
#endif

    expose
    std::string Name;
    uint32_t UniqueID;
    uint32_t Width;
    uint32_t Height;
    bool IsCubemap;
    uint32_t Mips;
    RhiBindFlags BindFlags;
    RhiFormat Format;

#ifdef __OBJC__
    expose
    static auto Create(id<MTLDevice> device, const Descriptor& desc) -> std::shared_ptr<MetalTexture>;

    auto GetNative() const -> id<MTLTexture> { return _texture; }
    auto GetDepthTexture() const -> id<MTLTexture> { return _depthTexture; }
#endif

    hide
#ifdef __OBJC__
    explicit MetalTexture(id<MTLDevice> device, const Descriptor& desc);
    auto CreateTexture2DResources(id<MTLDevice> device, const uint8_t* data) -> void;
    auto CreateCubemapResources(id<MTLDevice> device, const uint8_t* data) -> void;
#endif

    expose
    ~MetalTexture();
};

#endif
