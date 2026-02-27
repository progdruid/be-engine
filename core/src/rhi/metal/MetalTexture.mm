#ifdef __APPLE__

#import "MetalTexture.h"
#import "MetalFormatConverter.h"
#import <Metal/Metal.h>

static uint32_t s_nextTextureID = 1;

auto MetalTexture::Create(id<MTLDevice> device, const Descriptor& desc) -> std::shared_ptr<MetalTexture> {
    return std::shared_ptr<MetalTexture>(new MetalTexture(device, desc));
}

MetalTexture::MetalTexture(id<MTLDevice> device, const Descriptor& desc) {
    Name = desc.Name;
    UniqueID = s_nextTextureID++;
    Width = desc.Width;
    Height = desc.Height;
    IsCubemap = desc.IsCubemap;
    Mips = desc.Mips;
    BindFlags = desc.BindFlags;
    Format = desc.Format;

    if (IsCubemap) {
        CreateCubemapResources(device, desc.Data);
    } else {
        CreateTexture2DResources(device, desc.Data);
    }
}

MetalTexture::~MetalTexture() {
    _texture = nil;
    _depthTexture = nil;
}

auto MetalTexture::CreateTexture2DResources(id<MTLDevice> device, const uint8_t* data) -> void {
    MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MetalFormatConverter::ToMTLPixelFormat(Format)
                                                                                      width:Width
                                                                                     height:Height
                                                                                  mipmapped:(Mips > 1)];
    texDesc.mipmapLevelCount = Mips;

    MTLTextureUsage usage = MTLTextureUsageShaderRead;
    MTLStorageMode storage = MTLStorageModePrivate;

    if (HasFlag(BindFlags, RhiBindFlags::RenderTarget)) {
        usage |= MTLTextureUsageRenderTarget;
        storage = MTLStorageModePrivate;
    }
    if (HasFlag(BindFlags, RhiBindFlags::ShaderResource)) {
        usage |= MTLTextureUsageShaderRead;
    }

    texDesc.usage = usage;
    texDesc.storageMode = storage;

    _texture = [device newTextureWithDescriptor:texDesc];

    if (data && storage != MTLStorageModePrivate) {
        NSUInteger bytesPerRow = Width * 4;
        [_texture replaceRegion:MTLRegionMake2D(0, 0, Width, Height)
                    mipmapLevel:0
                      withBytes:data
                    bytesPerRow:bytesPerRow];
    }

    if (HasFlag(BindFlags, RhiBindFlags::DepthStencil)) {
        MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                            width:Width
                                                                                           height:Height
                                                                                        mipmapped:NO];
        depthDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        depthDesc.storageMode = MTLStorageModePrivate;
        _depthTexture = [device newTextureWithDescriptor:depthDesc];
    }
}

auto MetalTexture::CreateCubemapResources(id<MTLDevice> device, const uint8_t* data) -> void {
    MTLTextureDescriptor* texDesc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MetalFormatConverter::ToMTLPixelFormat(Format)
                                                                                         size:Width
                                                                                    mipmapped:(Mips > 1)];
    texDesc.mipmapLevelCount = Mips;

    MTLTextureUsage usage = MTLTextureUsageShaderRead;
    if (HasFlag(BindFlags, RhiBindFlags::RenderTarget)) {
        usage |= MTLTextureUsageRenderTarget;
    }

    texDesc.usage = usage;
    texDesc.storageMode = MTLStorageModePrivate;

    _texture = [device newTextureWithDescriptor:texDesc];

    if (HasFlag(BindFlags, RhiBindFlags::DepthStencil)) {
        MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                               size:Width
                                                                                          mipmapped:NO];
        depthDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        depthDesc.storageMode = MTLStorageModePrivate;
        _depthTexture = [device newTextureWithDescriptor:depthDesc];
    }
}

#endif
