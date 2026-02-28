#include "BeTexture.h"
#include "BeRenderer.h"
#include "BeTextureImpl.h"
#include "BeRendererImpl.h"
#include "MetalUtils.h"

#import <Metal/Metal.h>

static auto CreateTexture2DResources(
    id<MTLDevice> device,
    BeTextureImpl& impl,
    uint32_t width, uint32_t height, uint32_t mips,
    BeTextureFormat format, BeBindFlags bindFlags,
    const uint8_t* defaultData
) -> void {
    bool isDepth = MetalUtils::HasDepthComponent(format);

    if (HasAny(bindFlags, BeBindFlags::DepthStencil)) {
        MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MetalUtils::ToDepthPixelFormat(format)
                                                                                            width:width
                                                                                           height:height
                                                                                        mipmapped:NO];
        depthDesc.usage = MTLTextureUsageRenderTarget;
        depthDesc.storageMode = MTLStorageModePrivate;
        impl.depthTexture = [device newTextureWithDescriptor:depthDesc];
    }

    MTLPixelFormat pixelFormat = isDepth ? MetalUtils::ToShaderReadPixelFormat(format) : MetalUtils::ToMTLPixelFormat(format);

    MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                      width:width
                                                                                     height:height
                                                                                  mipmapped:(mips > 1)];
    texDesc.mipmapLevelCount = mips;
    MTLTextureUsage usage = 0;
    if (HasAny(bindFlags, BeBindFlags::ShaderResource)) usage |= MTLTextureUsageShaderRead;
    if (HasAny(bindFlags, BeBindFlags::RenderTarget))   usage |= MTLTextureUsageRenderTarget;
    texDesc.usage = usage;
    texDesc.storageMode = MTLStorageModePrivate;

    if (defaultData && !isDepth) {
        texDesc.storageMode = MTLStorageModeManaged;
    }

    impl.texture = [device newTextureWithDescriptor:texDesc];

    if (defaultData && !isDepth) {
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [impl.texture replaceRegion:region mipmapLevel:0 withBytes:defaultData bytesPerRow:4 * width];
    }

    if (HasAny(bindFlags, BeBindFlags::RenderTarget)) {
        impl.mipRenderTargetViews.resize(mips);
        for (uint32_t mip = 0; mip < mips; ++mip) {
            impl.mipRenderTargetViews[mip] = [impl.texture newTextureViewWithPixelFormat:pixelFormat
                                                                             textureType:MTLTextureType2D
                                                                                  levels:NSMakeRange(mip, 1)
                                                                                  slices:NSMakeRange(0, 1)];
        }
    }
}

static auto CreateCubemapResources(
    id<MTLDevice> device,
    BeTextureImpl& impl,
    uint32_t width, uint32_t height, uint32_t mips,
    BeTextureFormat format, BeBindFlags bindFlags,
    const uint8_t* defaultData
) -> void {
    bool isDepth = MetalUtils::HasDepthComponent(format);
    MTLPixelFormat pixelFormat = isDepth ? MetalUtils::ToShaderReadPixelFormat(format) : MetalUtils::ToMTLPixelFormat(format);

    MTLTextureDescriptor* texDesc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:pixelFormat
                                                                                         size:width
                                                                                    mipmapped:(mips > 1)];
    texDesc.mipmapLevelCount = mips;
    MTLTextureUsage usage = 0;
    if (HasAny(bindFlags, BeBindFlags::ShaderResource)) usage |= MTLTextureUsageShaderRead;
    if (HasAny(bindFlags, BeBindFlags::RenderTarget))   usage |= MTLTextureUsageRenderTarget;
    if (HasAny(bindFlags, BeBindFlags::DepthStencil))   usage |= MTLTextureUsageRenderTarget;
    texDesc.usage = usage;
    texDesc.storageMode = MTLStorageModePrivate;

    impl.texture = [device newTextureWithDescriptor:texDesc];

    if (HasAny(bindFlags, BeBindFlags::DepthStencil)) {
        MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MetalUtils::ToDepthPixelFormat(format)
                                                                                               size:width
                                                                                          mipmapped:NO];
        depthDesc.usage = MTLTextureUsageRenderTarget;
        depthDesc.storageMode = MTLStorageModePrivate;
        impl.depthTexture = [device newTextureWithDescriptor:depthDesc];

        for (uint32_t face = 0; face < 6; face++) {
            impl.cubemapDepthViews[face] = [impl.depthTexture newTextureViewWithPixelFormat:MetalUtils::ToDepthPixelFormat(format)
                                                                                textureType:MTLTextureType2D
                                                                                     levels:NSMakeRange(0, 1)
                                                                                     slices:NSMakeRange(face, 1)];
        }
    }

    if (HasAny(bindFlags, BeBindFlags::RenderTarget)) {
        for (uint32_t face = 0; face < 6; face++) {
            impl.cubemapMipRenderTargetViews[face].resize(mips);
            for (uint32_t mip = 0; mip < mips; ++mip) {
                impl.cubemapMipRenderTargetViews[face][mip] = [impl.texture newTextureViewWithPixelFormat:pixelFormat
                                                                                             textureType:MTLTextureType2D
                                                                                                  levels:NSMakeRange(mip, 1)
                                                                                                  slices:NSMakeRange(face, 1)];
            }
        }
    }
}

BeTexture::~BeTexture() = default;

auto BeTexture::CreatePlatformResources(BeRenderer& renderer, const uint8_t* initialData) -> void {
    _impl = std::make_unique<BeTextureImpl>();
    auto device = renderer.GetPlatformImpl()->device;

    if (!IsCubemap) {
        CreateTexture2DResources(device, *_impl, Width, Height, Mips, Format, BindFlags, initialData);
    } else {
        CreateCubemapResources(device, *_impl, Width, Height, Mips, Format, BindFlags, initialData);
    }
}
