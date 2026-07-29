#pragma once
#include <string>

#include <umbrellas/common.hpp>
#include <sen-rhi/SenCommandBuffer.h>

class BeRenderer;

class BeRenderPass {
    expose
    virtual ~BeRenderPass() = default;

    virtual auto Initialise(BeRenderer& renderer) -> void = 0;
    virtual auto Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void = 0;
    virtual auto GetPassName() const -> const std::string { return "RenderPass"; }
};
