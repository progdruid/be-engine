#pragma once

#include <memory>
#include <cstdint>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BeBuffers.h"

class BeWindow;
class BePipeline;
class BeRenderPass;
struct BeRendererImpl;

class BeRenderer {

    expose struct DrawEntry {
        glm::vec3 Position = {0.f, 0.f, 0.f};
        glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
        glm::vec3 Scale = {1.f, 1.f, 1.f};
        std::shared_ptr<struct BeModel> Model = nullptr;
        bool CastShadows = true;
    };

    expose BeUniformData UniformData;

    hide
    uint32_t _width;
    uint32_t _height;
    std::unique_ptr<BeRendererImpl> _impl;
    std::shared_ptr<BePipeline> _pipeline = nullptr;
    std::vector<BeRenderPass*> _passes;

    expose
    explicit BeRenderer(
        uint32_t width,
        uint32_t height,
        BeWindow& window
    );
    ~BeRenderer();

    auto LaunchDevice() -> void;

    expose
    auto AddRenderPass(BeRenderPass* renderPass) -> void;
    auto ClearPasses() -> void;
    auto InitialisePasses() const -> void;
    auto Render() -> void;

    [[nodiscard]] auto GetPipeline() const -> std::shared_ptr<BePipeline> { return _pipeline; }
    [[nodiscard]] auto GetWidth() const -> uint32_t { return _width; }
    [[nodiscard]] auto GetHeight() const -> uint32_t { return _height; }

    expose auto GetPlatformImpl() const -> BeRendererImpl* { return _impl.get(); }
};
