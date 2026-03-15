#pragma once

#include <vector>
#include <memory>
#include <umbrellas/access-modifiers.hpp>

#include <sen-rhi/SenCommandBuffer.h>
#include <sen-rhi/SenTypes.h>

class BeWindow;
class BeRenderPass;
class BeShader;


class BeRenderer {
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    uint32_t _width;
    uint32_t _height;
    void* _nativeWindow;

    SenSwapchain _swapchain;
    SenTexture _backbufferTexture;
    SenCommandBuffer _commandBuffer;

    std::vector<BeRenderPass*> _passes;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    explicit BeRenderer(
        uint32_t width,
        uint32_t height,
        void* nativeWindow
    );
    ~BeRenderer();

    auto LaunchDevice () -> void;


    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////

    expose
    auto AddRenderPass(BeRenderPass* renderPass) -> void;
    auto ClearPasses() -> void;
    auto InitialisePasses() -> void;
    auto Render() -> void;

    [[nodiscard]] auto GetCommandBuffer () -> SenCommandBuffer& { return _commandBuffer; }
    [[nodiscard]] auto GetBackbufferTexture() const -> SenTexture { return _backbufferTexture; }

    [[nodiscard]] auto GetWidth () const -> uint32_t { return _width; }
    [[nodiscard]] auto GetHeight () const -> uint32_t { return _height; }
    [[nodiscard]] auto GetViewport () const -> SenViewport {
        return { 0, 0, (float)_width, (float)_height, 0, 1 };
    }
    [[nodiscard]] auto GetSwapchainFormat() const -> SenFormat;
};
