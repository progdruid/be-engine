#include "BeAssetRegistry.h"
#include "BeRendererImpl.h"
#include "MetalUtils.h"

#include "BeRenderer.h"
#include "BeShaderTools.h"

#import <Metal/Metal.h>

struct BeSamplerImpl {
    id<MTLSamplerState> samplerState;
};

auto BeAssetRegistry::GetSampler(std::string_view samplerDescString) -> BeSampler {
    auto key = std::string(samplerDescString);

    if (_samplers.contains(key)) {
        return _samplers[key];
    }

    auto tokens = BeShaderTools::Split(samplerDescString, "-");
    be_assert(
        tokens.size() == 2 || tokens.size() == 3,
        "Invalid samplerDescString. Expected format: filter-address[-cmp]",
        samplerDescString,
        tokens.size()
    );

    auto filterToken = std::string(tokens[0]);
    auto addressToken = std::string(tokens[1]);
    auto hasComparison = tokens.size() == 3 && tokens[2] == "cmp";

    MTLSamplerMinMagFilter minMagFilter = MTLSamplerMinMagFilterLinear;
    MTLSamplerMipFilter mipFilter = MTLSamplerMipFilterLinear;
    if (filterToken == "point") {
        minMagFilter = MTLSamplerMinMagFilterNearest;
        mipFilter = MTLSamplerMipFilterNearest;
    } else if (filterToken == "linear") {
        minMagFilter = MTLSamplerMinMagFilterLinear;
        mipFilter = MTLSamplerMipFilterLinear;
    } else {
        be_assert(false, "Unknown filter token", filterToken);
    }

    MTLSamplerAddressMode addressMode = MTLSamplerAddressModeClampToEdge;
    if (addressToken == "wrap") {
        addressMode = MTLSamplerAddressModeRepeat;
    } else if (addressToken == "clamp") {
        addressMode = MTLSamplerAddressModeClampToEdge;
    } else if (addressToken == "mirror") {
        addressMode = MTLSamplerAddressModeMirrorRepeat;
    } else {
        be_assert(false, "Unknown address token", addressToken);
    }

    MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
    samplerDesc.minFilter = minMagFilter;
    samplerDesc.magFilter = minMagFilter;
    samplerDesc.mipFilter = mipFilter;
    samplerDesc.sAddressMode = addressMode;
    samplerDesc.tAddressMode = addressMode;
    samplerDesc.rAddressMode = addressMode;
    samplerDesc.maxAnisotropy = 1;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = FLT_MAX;

    if (hasComparison) {
        samplerDesc.compareFunction = MTLCompareFunctionLess;
    }

    auto renderer = _renderer.lock();
    be_assert(renderer, "Renderer couldn't be locked");

    auto device = renderer->GetPlatformImpl()->device;
    auto samplerState = std::make_shared<BeSamplerImpl>();

    samplerState->samplerState = [device newSamplerStateWithDescriptor:samplerDesc];

    _samplers[key] = samplerState;
    return samplerState;
}
