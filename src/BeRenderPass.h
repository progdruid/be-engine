#pragma once
#include <string>
#include <vector>
#include <wrl/client.h>

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
    
    [[nodiscard]] virtual auto GetInputResources() const -> std::vector<std::string> = 0;
    [[nodiscard]] virtual auto GetOutputResources() const -> std::vector<std::string> = 0;

    virtual auto Initialise() -> void = 0;
    virtual auto Render() -> void = 0;
};
