#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include <umbrellas/access-modifiers.hpp>

#include "RhiTypes.h"

class RhiBuffer;
class RhiSampler;

struct RhiBufferDesc {
    uint32_t ByteWidth = 0;
    RhiBufferUsage Usage = RhiBufferUsage::Default;
    RhiBindFlags BindFlags = RhiBindFlags::None;
    bool CPUWriteAccess = false;
};

struct RhiSamplerDesc {
    RhiFilter Filter = RhiFilter::Linear;
    RhiAddressMode AddressU = RhiAddressMode::Wrap;
    RhiAddressMode AddressV = RhiAddressMode::Wrap;
    RhiAddressMode AddressW = RhiAddressMode::Wrap;
    RhiComparisonFunc ComparisonFunc = RhiComparisonFunc::Never;
    bool UseComparison = false;
    uint32_t MaxAnisotropy = 1;
    float MipLODBias = 0;
    float MinLOD = 0;
    float MaxLOD = 3.402823466e+38f;
};

struct RhiRasterizerDesc {
    RhiCullMode CullMode = RhiCullMode::Back;
    bool FrontCounterClockwise = false;
    bool DepthClipEnable = true;
    bool Wireframe = false;
};

struct RhiDepthStencilDesc {
    bool DepthEnable = true;
    bool DepthWriteAll = true;
    RhiComparisonFunc DepthFunc = RhiComparisonFunc::Less;
    bool StencilEnable = false;
};

struct RhiBlendDesc {
    bool BlendEnable = false;
};

class RhiDevice {
    expose
    virtual ~RhiDevice() = default;

    virtual auto CreateBuffer(const RhiBufferDesc& desc, const void* initialData = nullptr) -> std::shared_ptr<RhiBuffer> = 0;
    virtual auto CreateSampler(const RhiSamplerDesc& desc) -> std::shared_ptr<RhiSampler> = 0;

    virtual auto GetBackendName() const -> const char* = 0;
};
