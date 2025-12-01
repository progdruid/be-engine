#pragma once
#include <wrl/client.h>
#include <string>

#include "umbrellas/access-modifiers.hpp"

using Microsoft::WRL::ComPtr;

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
    virtual auto Render() -> void = 0;
    virtual auto GetPassName() const -> const std::string { return "RenderPass"; }
};
