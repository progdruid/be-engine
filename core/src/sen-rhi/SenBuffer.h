#pragma once
#include <cstdint>
#include <sen-rhi/SenTypes.h>

struct SenBuffer {
    uint32_t id = 0;
    auto IsValid() const -> bool { return id != 0; }
};

struct SenBufferDesc {
    SenBufferUsage  usage  = SenBufferUsage::Constant;
    SenBufferAccess access = SenBufferAccess::Dynamic;
    uint32_t        size   = 0;       // in bytes
    const void*     data   = nullptr; // optional initial data, not owned
};
