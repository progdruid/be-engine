#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <glm.hpp>
#include <memory>
#include <wrl/client.h>

#include "BeModel.h"

class BeShader;
class ModelAsset;
using Microsoft::WRL::ComPtr;

class Renderer {

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        //glm::vec2 uv;
        //glm::vec3 normal;
    };
    
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
    //glm::vec3 ClearColor = {53.f / 255.f, 144.f / 255.f, 243.f / 255.f}; // blue
    glm::vec3 ClearColor = {255.f / 255.f, 205.f / 255.f, 27.f / 255.f}; // gold
    
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
    ComPtr<ID3D11DepthStencilView> _depthStencilView;
    ComPtr<ID3D11DepthStencilState> _depthStencilState;
    std::shared_ptr<BeShader> _shader;
    std::shared_ptr<BeModel> _model; 

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
