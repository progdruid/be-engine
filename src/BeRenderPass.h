#pragma once
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

class BeRenderer;

class BeRenderPass {
protected:
    BeRenderer* _renderer = nullptr;

public:
    virtual ~BeRenderPass() = default;

    auto InjectRenderer (BeRenderer* renderer) -> void {
        _renderer = renderer;
    }

    virtual auto Initialise() -> void = 0;
    virtual auto Render() -> void = 0;
    virtual auto GetPassName() const -> const std::string { return "RenderPass"; }
};
