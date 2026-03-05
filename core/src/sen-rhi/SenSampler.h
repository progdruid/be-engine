#pragma once
#include <cstdint>
#include <sen-rhi/SenTypes.h>

struct SenSampler {
    uint32_t id = 0;
    auto IsValid() const -> bool { return id != 0; }
};

struct SenSamplerDesc {
    SenFilter      filter     = SenFilter::Linear;
    SenAddressMode address    = SenAddressMode::Clamp; // applied to U, V, and W
    bool           comparison = false;                 // enables less-than comparison (shadow maps)
};
