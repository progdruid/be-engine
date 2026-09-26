#pragma once
#include <string>

#include <common.hpp>
#include <SenTypes.h>

class BeRenderer;

class BeRenderPass {
    expose
    virtual ~BeRenderPass() = default;

    virtual auto Initialise(BeRenderer& renderer) -> void = 0;
    virtual auto Render(BeRenderer& renderer, SenCommandList cmd) -> void = 0;
    virtual auto GetPassName() const -> const std::string { return "RenderPass"; }
};
