#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <vector>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <umbrellas/include-glm.h>

#include "BeBuffers.h"
#include "umbrellas/access-modifiers.hpp"


struct BeDrawSlice;
struct BeModel;
class BeAssetRegistry;
class BePipeline;
class BeRenderPass;
class BeShader;
using Microsoft::WRL::ComPtr;


class BeRenderer {

    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose struct ObjectEntry {
        glm::vec3 Position = {0.f, 0.f, 0.f};
        glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
        glm::vec3 Scale = {1.f, 1.f, 1.f};
        std::shared_ptr<BeModel> Model = nullptr;
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
    std::shared_ptr<BePipeline> _pipeline = nullptr;

    ComPtr<ID3D11Buffer> _uniformBuffer;
    ComPtr<ID3D11SamplerState> _pointSampler;
    ComPtr<ID3D11SamplerState> _postProcessLinearClampSampler;
    ComPtr<ID3D11DepthStencilState> _defaultDepthStencilState;

    ComPtr<ID3D11Buffer> _sharedVertexBuffer;
    ComPtr<ID3D11Buffer> _sharedIndexBuffer;
    std::unordered_map<BeModel*, std::vector<BeDrawSlice>> _modelDrawSlices;
    std::vector<ObjectEntry> _objectsToDraw;
    
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
    [[nodiscard]] auto GetPipeline() const -> std::shared_ptr<BePipeline> { return _pipeline; }
    [[nodiscard]] auto GetAssetRegistry() const -> std::weak_ptr<BeAssetRegistry> { return _assetRegistry; }
    [[nodiscard]] auto GetPointSampler() const -> ComPtr<ID3D11SamplerState> { return _pointSampler; }
    [[nodiscard]] auto GetPostProcessLinearClampSampler() const -> ComPtr<ID3D11SamplerState> { return _postProcessLinearClampSampler; }
    [[nodiscard]] auto GetBackbufferTarget() const -> ComPtr<ID3D11RenderTargetView> { return _backbufferTarget; }

    [[nodiscard]] auto GetWidth () const -> uint32_t { return _width; }
    [[nodiscard]] auto GetHeight () const -> uint32_t { return _height; }

    
    auto SetModels(const std::vector<std::shared_ptr<BeModel>>& models) -> void;
    auto GetDrawSlicesForModel(const std::shared_ptr<BeModel>& model) -> const std::vector<BeDrawSlice>& { return _modelDrawSlices.at(model.get()); }

    auto SubmitObject(const ObjectEntry& object) -> void { _objectsToDraw.push_back(object); }
    auto GetObjectsToDraw() -> std::vector<ObjectEntry>& { return _objectsToDraw; }
    
    auto GetShaderVertexBuffer() -> ComPtr<ID3D11Buffer> { return _sharedVertexBuffer; }
    auto GetShaderIndexBuffer() -> ComPtr<ID3D11Buffer> { return _sharedIndexBuffer; }
};
