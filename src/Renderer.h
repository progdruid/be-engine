#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <glm.hpp>
#include <memory>
#include <wrl/client.h>

#include "Shader.h"
using Microsoft::WRL::ComPtr;

class Renderer {

    struct alignas(16) UniformData {
        glm::mat4x4 ProjectionView;
    };
    
    struct alignas(16) ObjectData {
        glm::mat4x4 Model;
    };
    
public:
    explicit Renderer(HWND windowHandle, int width, int height);
    ~Renderer() = default;

public:
    glm::vec3 ClearColor = {0.071f, 0.04f, 0.561f};
    
private:
    bool _active = false;

    HWND _windowHandle;
    int _width;
    int _height;
    
    ComPtr<ID3D11Device> _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGIDevice> _dxgiDevice;
    ComPtr<IDXGIAdapter> _adapter;
    ComPtr<IDXGIFactory2> _factory;
    ComPtr<IDXGISwapChain1> _swapchain;
    ComPtr<ID3D11RenderTargetView> _renderTarget;
    std::shared_ptr<Shader> _shader;
    ComPtr<ID3D11Buffer> _vertexBuffer;
    ComPtr<ID3D11Buffer> _indexBuffer;
    
    glm::vec3 _cameraPos = {3.0f, 3.0f, -3.0f};
    glm::vec3 _cameraDirection = {-1.0f, -1.0f, 1.0f};
    glm::float32 _fov = 45.0f;
    UniformData _uniformData;
    ComPtr<ID3D11Buffer> _uniformBuffer;
    
    glm::vec3 _objectPos = {0.5f, 0.25f, 0.0f};
    ComPtr<ID3D11Buffer> _objectBuffer;

public:
    [[nodiscard]] auto isActive() const -> bool { return _active; }

    auto render() -> void;

    auto setProjectionView(const glm::mat4x4& projectionView) -> void;
    
private:
    auto terminateRenderer() -> void;
};
