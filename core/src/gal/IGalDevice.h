#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include <umbrellas/access-modifiers.hpp>

#include "GalTypes.h"

class IGalBuffer;
class IGalSampler;

struct GalBufferDesc {
    uint32_t ByteWidth = 0;
    GalBufferUsage Usage = GalBufferUsage::Default;
    GalBindFlags BindFlags = GalBindFlags::None;
    bool CPUWriteAccess = false;
};

struct GalSamplerDesc {
    GalFilter Filter = GalFilter::Linear;
    GalAddressMode AddressU = GalAddressMode::Wrap;
    GalAddressMode AddressV = GalAddressMode::Wrap;
    GalAddressMode AddressW = GalAddressMode::Wrap;
    GalComparisonFunc ComparisonFunc = GalComparisonFunc::Never;
    bool UseComparison = false;
    uint32_t MaxAnisotropy = 1;
    float MipLODBias = 0;
    float MinLOD = 0;
    float MaxLOD = 3.402823466e+38f;
};

struct GalRasterizerDesc {
    GalCullMode CullMode = GalCullMode::Back;
    bool FrontCounterClockwise = false;
    bool DepthClipEnable = true;
    bool Wireframe = false;
};

struct GalDepthStencilDesc {
    bool DepthEnable = true;
    bool DepthWriteAll = true;
    GalComparisonFunc DepthFunc = GalComparisonFunc::Less;
    bool StencilEnable = false;
};

struct GalBlendDesc {
    bool BlendEnable = false;
};

class IGalDevice {
    expose
    virtual ~IGalDevice() = default;

    virtual auto CreateBuffer(const GalBufferDesc& desc, const void* initialData = nullptr) -> std::shared_ptr<IGalBuffer> = 0;
    virtual auto CreateSampler(const GalSamplerDesc& desc) -> std::shared_ptr<IGalSampler> = 0;

    virtual auto GetBackendName() const -> const char* = 0;
};
