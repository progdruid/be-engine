#pragma once

#include <vector>
#include <umbrellas/access-modifiers.hpp>

#include <sen-rhi/SenCommandBuffer.h>
#include <sen-rhi/SenTypes.h>

class BeWindow;
class BeRenderPass;
class BeShader;


class BeRenderer {
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    uint32_t _desiredWidth;
    uint32_t _desiredHeight;
    void* _nativeWindow;
    SenSwapchain _swapchain;

    SenTexture _backbufferTexture;
    SenCommandBuffer _frameCmd;

    std::vector<BeRenderPass*> _passes;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    explicit BeRenderer(
        uint32_t desiredWidth,
        uint32_t desiredHeight,
        void* nativeWindow
    );
    ~BeRenderer();

    auto LaunchDevice () -> void;
    
    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto AddRenderPass(BeRenderPass* renderPass) -> void;
    auto ClearPasses() -> void;
    auto Render() -> void;
    auto RenderOnce(const std::vector<BeRenderPass*>& passes) -> void;

    [[nodiscard]] auto GetBackbufferTexture() const -> SenTexture { return _backbufferTexture; }

    [[nodiscard]] auto GetSwapchainPixelWidth () const -> uint32_t;
    [[nodiscard]] auto GetSwapchainPixelHeight () const -> uint32_t;
    [[nodiscard]] auto GetViewport () const -> SenViewport;
    [[nodiscard]] auto GetSwapchainFormat() const -> SenFormat;
};
