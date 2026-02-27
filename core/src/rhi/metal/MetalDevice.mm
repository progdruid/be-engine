#ifdef __APPLE__

#import "MetalDevice.h"
#import "MetalBuffer.h"
#import "MetalSampler.h"
#import <Metal/Metal.h>

MetalDevice::MetalDevice() {
    _device = MTLCreateSystemDefaultDevice();
}

MetalDevice::~MetalDevice() {
    _device = nil;
}

auto MetalDevice::CreateBuffer(const RhiBufferDesc& desc, const void* initialData) -> std::shared_ptr<RhiBuffer> {
    MTLResourceOptions options;

    switch (desc.Usage) {
        case RhiBufferUsage::Dynamic:
            options = MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined;
            break;
        case RhiBufferUsage::Immutable:
            options = MTLResourceStorageModePrivate;
            break;
        default:
            options = MTLResourceStorageModeShared;
            break;
    }

    id<MTLBuffer> buffer;
    if (initialData) {
        buffer = [_device newBufferWithBytes:initialData length:desc.ByteWidth options:options];
    } else {
        buffer = [_device newBufferWithLength:desc.ByteWidth options:options];
    }

    if (!buffer) return nullptr;

    return std::make_shared<MetalBuffer>(buffer, desc.ByteWidth, desc.Usage, desc.BindFlags);
}

auto MetalDevice::CreateSampler(const RhiSamplerDesc& desc) -> std::shared_ptr<RhiSampler> {
    MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];

    auto toMinMagFilter = [](RhiFilter filter) -> MTLSamplerMinMagFilter {
        switch (filter) {
            case RhiFilter::Point:  return MTLSamplerMinMagFilterNearest;
            default:                return MTLSamplerMinMagFilterLinear;
        }
    };

    auto toMipFilter = [](RhiFilter filter) -> MTLSamplerMipFilter {
        switch (filter) {
            case RhiFilter::Point:  return MTLSamplerMipFilterNearest;
            default:                return MTLSamplerMipFilterLinear;
        }
    };

    auto toAddressMode = [](RhiAddressMode mode) -> MTLSamplerAddressMode {
        switch (mode) {
            case RhiAddressMode::Clamp:  return MTLSamplerAddressModeClampToEdge;
            case RhiAddressMode::Mirror: return MTLSamplerAddressModeMirrorRepeat;
            case RhiAddressMode::Border: return MTLSamplerAddressModeClampToBorderColor;
            default:                     return MTLSamplerAddressModeRepeat;
        }
    };

    samplerDesc.minFilter = toMinMagFilter(desc.Filter);
    samplerDesc.magFilter = toMinMagFilter(desc.Filter);
    samplerDesc.mipFilter = toMipFilter(desc.Filter);
    samplerDesc.sAddressMode = toAddressMode(desc.AddressU);
    samplerDesc.tAddressMode = toAddressMode(desc.AddressV);
    samplerDesc.rAddressMode = toAddressMode(desc.AddressW);
    samplerDesc.maxAnisotropy = desc.MaxAnisotropy;
    samplerDesc.lodMinClamp = desc.MinLOD;
    samplerDesc.lodMaxClamp = desc.MaxLOD;

    if (desc.UseComparison) {
        auto toCompareFunc = [](RhiComparisonFunc func) -> MTLCompareFunction {
            switch (func) {
                case RhiComparisonFunc::Never:        return MTLCompareFunctionNever;
                case RhiComparisonFunc::Less:         return MTLCompareFunctionLess;
                case RhiComparisonFunc::Equal:        return MTLCompareFunctionEqual;
                case RhiComparisonFunc::LessEqual:    return MTLCompareFunctionLessEqual;
                case RhiComparisonFunc::Greater:      return MTLCompareFunctionGreater;
                case RhiComparisonFunc::NotEqual:     return MTLCompareFunctionNotEqual;
                case RhiComparisonFunc::GreaterEqual: return MTLCompareFunctionGreaterEqual;
                case RhiComparisonFunc::Always:       return MTLCompareFunctionAlways;
                default:                              return MTLCompareFunctionNever;
            }
        };
        samplerDesc.compareFunction = toCompareFunc(desc.ComparisonFunc);
    }

    id<MTLSamplerState> sampler = [_device newSamplerStateWithDescriptor:samplerDesc];
    if (!sampler) return nullptr;

    return std::make_shared<MetalSampler>(sampler);
}

#endif
