#pragma once
#include <string>

#include <umbrellas/common.hpp>
#include <sen-rhi/SenCommandBuffer.h>

class BeRenderer;

class BeRenderPass {
    protect
    BeRenderer* _renderer = nullptr;

    expose
    virtual ~BeRenderPass() = default;

    auto InjectRenderer (BeRenderer* renderer) -> void {
        _renderer = renderer;
    }

    virtual auto Initialise() -> void = 0;
    virtual auto Render(SenCommandBuffer& cmd) -> void = 0;
    virtual auto GetPassName() const -> const std::string { return "RenderPass"; }
};
