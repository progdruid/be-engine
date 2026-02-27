#pragma once
#include <umbrellas/access-modifiers.hpp>

class RhiSwapchain {
    expose
    virtual ~RhiSwapchain() = default;

    virtual auto Present(uint32_t syncInterval = 1) -> void = 0;
    virtual auto Resize(uint32_t width, uint32_t height) -> void = 0;
};
