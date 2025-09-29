#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <memory>
#include <vector>
#include <wrl/client.h>

#include "BeModel.h"

class BeShader;
class ModelAsset;
using Microsoft::WRL::ComPtr;

class Renderer {
    struct alignas(16) UniformBufferData {
        glm::mat4x4 ProjectionView;
    };

    struct alignas(16) ObjectBufferData {
        glm::mat4x4 Model;
    };

    struct ObjectEntry {
        std::string Name;
        glm::vec3 Position = {0.f, 0.f, 0.f};
        glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
        glm::vec3 Scale = {1.f, 1.f, 1.f};
        BeModel& Model;
        std::vector<BeModel::BeMeshInstruction> MeshInstructions;
        std::shared_ptr<BeShader> Shader; // optional override
    };

public:
    explicit Renderer(HWND windowHandle, uint32_t width, uint32_t height);
    ~Renderer() = default;

public:
    glm::vec3 ClearColor = {53.f / 255.f, 144.f / 255.f, 243.f / 255.f}; // blue
    //glm::vec3 ClearColor = {255.f / 255.f, 205.f / 255.f, 27.f / 255.f}; // gold

private:
    bool _active = false;

    HWND _windowHandle;
    uint32_t _width;
    uint32_t _height;

    ComPtr<ID3D11Device> _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGIDevice> _dxgiDevice;
    ComPtr<IDXGIAdapter> _adapter;
    ComPtr<IDXGIFactory2> _factory;
    ComPtr<IDXGISwapChain1> _swapchain;

    ComPtr<ID3D11RenderTargetView> _renderTarget;
    ComPtr<ID3D11DepthStencilView> _depthStencilView;
    ComPtr<ID3D11DepthStencilState> _depthStencilState;

    UniformBufferData _uniformData;
    ComPtr<ID3D11Buffer> _uniformBuffer;

    glm::vec3 _objectPos = {0.5f, 0.25f, 0.0f};
    ComPtr<ID3D11Buffer> _objectBuffer;

    ComPtr<ID3D11Buffer> _sharedVertexBuffer;
    ComPtr<ID3D11Buffer> _sharedIndexBuffer;

    std::shared_ptr<BeShader> _colorShader;
    std::shared_ptr<BeShader> _texturedShader;
    ComPtr<ID3D11SamplerState> _pointSampler;

    std::vector<ObjectEntry> _objects;

public:
    [[nodiscard]] auto isActive() const -> bool { return _active; }

    auto render() -> void;

    auto setProjectionView(const glm::mat4x4& projectionView) -> void;

private:
    auto terminateRenderer() -> void;
};
