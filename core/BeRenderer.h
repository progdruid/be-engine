#pragma once

#include <array>
#include <vector>
#include <umbrellas/common.hpp>

#include <sen-rhi/SenCommandBuffer.h>
#include <sen-rhi/SenTypes.h>

class BeWindow;
class BeRenderPass;
class BeShader;


class BeRenderer {
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    static constexpr uint32_t FramesInFlight = 2;

    static auto GetCurrentFrame () -> uint64_t { return _currentFrame; }

    hide
    static uint64_t _currentFrame;

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    uint32_t _desiredWidth;
    uint32_t _desiredHeight;
    void* _nativeWindow;
    SenSwapchain _swapchain;

    SenTexture _backbufferTexture;
    std::array<SenCommandBuffer, FramesInFlight> _frameCmds;
    SenCommandBuffer _immediateCmd;

    std::vector<BeRenderPass*> _passes;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    explicit BeRenderer(
        uint32_t desiredWidth,
        uint32_t desiredHeight,
        void* nativeWindow
    );
    ~BeRenderer();

    auto LaunchDevice (SenPresentMode presentMode = SenPresentMode::VSync) -> void;
    
    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto SetPasses(std::vector<BeRenderPass*> passes) -> void;
    auto ClearPasses() -> void;
    auto Render() -> void;
    auto RenderOnce(const std::vector<BeRenderPass*>& passes) -> void;

    [[nodiscard]] auto GetBackbufferTexture() const -> SenTexture { return _backbufferTexture; }

    [[nodiscard]] auto GetSwapchainPixelWidth () const -> uint32_t;
    [[nodiscard]] auto GetSwapchainPixelHeight () const -> uint32_t;
    [[nodiscard]] auto GetViewport () const -> SenViewport;
    [[nodiscard]] auto GetSwapchainFormat() const -> SenFormat;
};
