#pragma once

#include <d3d11.h>
#include <vector>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>
#include <umbrellas/include-glm.h>

#include "BeModel.h"
#include "BeBuffers.h"
#include "BeTexture.h"
#include "BeShader.h"

class BeRenderPass;
class BeShader;
using Microsoft::WRL::ComPtr;


class BeRenderer {

    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose struct ObjectEntry {
        std::string Name;
        glm::vec3 Position = {0.f, 0.f, 0.f};
        glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
        glm::vec3 Scale = {1.f, 1.f, 1.f};
        std::shared_ptr<BeModel> Model = nullptr;
        std::vector<BeModel::BeDrawSlice> DrawSlices;
        bool CastShadows = true;
    };

    
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    hide static auto GetBestAdapter() -> ComPtr<IDXGIAdapter1>;
    
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose BeUniformData UniformData;

    hide
    uint32_t _width;
    uint32_t _height;
    HWND _windowHandle;
    std::weak_ptr<BeAssetRegistry> _assetRegistry;
    
    // dx11 core components
    ComPtr<ID3D11Device> _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGIDevice> _dxgiDevice;
    ComPtr<IDXGIAdapter> _adapter;
    ComPtr<IDXGIFactory2> _factory;
    ComPtr<IDXGISwapChain1> _swapchain;
    ComPtr<ID3D11RenderTargetView> _backbufferTarget;

    ComPtr<ID3D11Buffer> _uniformBuffer;
    ComPtr<ID3D11SamplerState> _pointSampler;
    ComPtr<ID3D11SamplerState> _postProcessLinearClampSampler;
    ComPtr<ID3D11DepthStencilState> _defaultDepthStencilState;

    ComPtr<ID3D11Buffer> _sharedVertexBuffer;
    ComPtr<ID3D11Buffer> _sharedIndexBuffer;
    std::vector<ObjectEntry> _objects;
    
    std::unordered_map<std::string, void*> _contextDataPointers;
    std::vector<BeRenderPass*> _passes;


    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    explicit BeRenderer(uint32_t width, uint32_t height, HWND windowHandle, std::weak_ptr<BeAssetRegistry> registry);
    ~BeRenderer();
    
    auto LaunchDevice () -> void;


    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    
    expose
    auto AddRenderPass(BeRenderPass* renderPass) -> void;
    auto InitialisePasses() const -> void;
    auto Render() -> void;

    [[nodiscard]] auto GetDevice() const -> ComPtr<ID3D11Device> { return _device; }
    [[nodiscard]] auto GetContext() const -> ComPtr<ID3D11DeviceContext> { return _context; }
    [[nodiscard]] auto GetAssetRegistry() const -> std::weak_ptr<BeAssetRegistry> { return _assetRegistry; }
    [[nodiscard]] auto GetPointSampler() const -> ComPtr<ID3D11SamplerState> { return _pointSampler; }
    [[nodiscard]] auto GetPostProcessLinearClampSampler() const -> ComPtr<ID3D11SamplerState> { return _postProcessLinearClampSampler; }
    [[nodiscard]] auto GetBackbufferTarget() const -> ComPtr<ID3D11RenderTargetView> { return _backbufferTarget; }

    [[nodiscard]] auto GetWidth () const -> uint32_t { return _width; }
    [[nodiscard]] auto GetHeight () const -> uint32_t { return _height; }
    
    auto SetObjects(const std::vector<ObjectEntry>& objects) -> void;
    auto GetObjects() -> const std::vector<ObjectEntry>& { return _objects; }
    auto GetShaderVertexBuffer() -> ComPtr<ID3D11Buffer> { return _sharedVertexBuffer; }
    auto GetShaderIndexBuffer() -> ComPtr<ID3D11Buffer> { return _sharedIndexBuffer; }

    template<typename T>
    auto GetContextDataPointer(const std::string& name) const -> T* {
        return static_cast<T*>(_contextDataPointers.at(name));
    }
    template<typename T>
    auto SetContextDataPointer(const std::string& name, T* data) -> void{
        _contextDataPointers[name] = static_cast<void*>(data);
    }
};
