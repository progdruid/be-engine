#pragma once
#include <cstdint>
#include <sen-rhi/SenTypes.h>

struct SenTexture {
    uint32_t id = 0;
    auto IsValid() const -> bool { return id != 0; }
};

struct SenTextureDesc {
    SenFormat       format  = SenFormat::Unknown;
    uint32_t        width   = 0;
    uint32_t        height  = 0;
    SenTextureUsage usage   = SenTextureUsage::None;
    uint32_t        mips    = 1;
    bool            cubemap = false;
    const uint8_t*  data    = nullptr; // optional initial pixel data, not owned
};
