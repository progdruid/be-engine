#pragma once

#include <array>
#include <vector>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

struct BeTextureImpl {
#ifdef __OBJC__
    id<MTLTexture> texture = nil;
    id<MTLTexture> depthTexture = nil;
    std::vector<id<MTLTexture>> mipRenderTargetViews;
    std::array<id<MTLTexture>, 6> cubemapDepthViews;
    std::array<std::vector<id<MTLTexture>>, 6> cubemapMipRenderTargetViews;
#else
    void* texture = nullptr;
    void* depthTexture = nullptr;
#endif
};
