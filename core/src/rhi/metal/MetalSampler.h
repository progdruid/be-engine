#pragma once
#ifdef __APPLE__

#include <umbrellas/access-modifiers.hpp>

#include "../RhiSampler.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

class MetalSampler final : public RhiSampler {

    hide
#ifdef __OBJC__
    id<MTLSamplerState> _sampler;
#else
    id _sampler;
#endif

    expose
#ifdef __OBJC__
    explicit MetalSampler(id<MTLSamplerState> sampler)
#else
    explicit MetalSampler(id sampler)
#endif
        : _sampler(sampler)
    {}

    ~MetalSampler() override = default;

#ifdef __OBJC__
    auto GetNative() const -> id<MTLSamplerState> { return _sampler; }
#endif
};

#endif
