#include "SenDx11Backend.h"

#include <unordered_map>
#include "Utils.h"
#include <umbrellas/include-libassert.h>
#include <sen-rhi/dx11/SenDx11Convert.h>
#include <sen-rhi/SenShaderCompiler.h>

// ─── texture helpers ──────────────────────────────────────────────────────────

static auto DepthSRVFormat(DXGI_FORMAT textureFormat) -> DXGI_FORMAT {
    static std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> map = {
        { DXGI_FORMAT_R32_TYPELESS,   DXGI_FORMAT_R32_FLOAT             },
        { DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS },
        { DXGI_FORMAT_R16_TYPELESS,   DXGI_FORMAT_R16_UNORM             },
    };
    return map.at(textureFormat);
}

static auto DepthDSVFormat(DXGI_FORMAT textureFormat) -> DXGI_FORMAT {
    static std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> map = {
        { DXGI_FORMAT_R32_TYPELESS,   DXGI_FORMAT_D32_FLOAT         },
        { DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT },
        { DXGI_FORMAT_R16_TYPELESS,   DXGI_FORMAT_D16_UNORM         },
    };
    return map.at(textureFormat);
}

static auto CreateTexture2D(
    const ComPtr<ID3D11Device>& device,
    const SenTextureDesc& desc,
    SenDx11TextureEntry& entry
) -> void {
    const DXGI_FORMAT dxFormat    = Sen::Dx11::ToFormat(desc.Format);
    const uint32_t    dxBindFlags = Sen::Dx11::ToBindFlags(desc.Usage);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = desc.Width;
    td.Height           = desc.Height;
    td.MipLevels        = desc.Mips;
    td.ArraySize        = 1;
    td.Format           = dxFormat;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = dxBindFlags;

    D3D11_SUBRESOURCE_DATA  initData = {};
    D3D11_SUBRESOURCE_DATA* pInit    = nullptr;
    if (desc.Data) {
        initData.pSysMem          = desc.Data;
        initData.SysMemPitch      = sizeof(uint8_t) * 4 * desc.Width;
        initData.SysMemSlicePitch = 0;
        pInit = &initData;
    }

    Utils::Check << device->CreateTexture2D(&td, pInit, entry.Texture.GetAddressOf());

    if (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format             = DepthDSVFormat(dxFormat);
        dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        Utils::Check << device->CreateDepthStencilView(entry.Texture.Get(), &dsvDesc, entry.DSV.GetAddressOf());
    }

    if (dxBindFlags & D3D11_BIND_RENDER_TARGET) {
        entry.MipRTVs.resize(desc.Mips);
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format        = dxFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        for (uint32_t mip = 0; mip < desc.Mips; ++mip) {
            rtvDesc.Texture2D.MipSlice = mip;
            Utils::Check << device->CreateRenderTargetView(entry.Texture.Get(), &rtvDesc, entry.MipRTVs[mip].GetAddressOf());
        }
    }

    if (dxBindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) ? DepthSRVFormat(dxFormat) : dxFormat;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels       = desc.Mips;
        Utils::Check << device->CreateShaderResourceView(entry.Texture.Get(), &srvDesc, entry.SRV.GetAddressOf());
    }
}

static auto CreateTextureCubemap(
    const ComPtr<ID3D11Device>& device,
    const SenTextureDesc& desc,
    SenDx11TextureEntry& entry
) -> void {
    const DXGI_FORMAT dxFormat    = Sen::Dx11::ToFormat(desc.Format);
    const uint32_t    dxBindFlags = Sen::Dx11::ToBindFlags(desc.Usage);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = desc.Width;
    td.Height           = desc.Height;
    td.MipLevels        = desc.Mips;
    td.ArraySize        = 6;
    td.Format           = dxFormat;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = dxBindFlags;
    td.MiscFlags        = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_SUBRESOURCE_DATA  initData = {};
    D3D11_SUBRESOURCE_DATA* pInit    = nullptr;
    if (desc.Data) {
        initData.pSysMem          = desc.Data;
        initData.SysMemPitch      = sizeof(uint8_t) * 4 * desc.Width;
        initData.SysMemSlicePitch = 0;
        pInit = &initData;
    }

    Utils::Check << device->CreateTexture2D(&td, pInit, entry.Texture.GetAddressOf());

    if (dxBindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                      = (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) ? DepthSRVFormat(dxFormat) : dxFormat;
        srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels       = desc.Mips;
        Utils::Check << device->CreateShaderResourceView(entry.Texture.Get(), &srvDesc, entry.SRV.GetAddressOf());
    }

    if (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) {
        for (uint32_t face = 0; face < 6; ++face) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format                         = DepthDSVFormat(dxFormat);
            dsvDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice        = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = face;
            dsvDesc.Texture2DArray.ArraySize       = 1;
            Utils::Check << device->CreateDepthStencilView(entry.Texture.Get(), &dsvDesc, entry.CubemapDSVs[face].GetAddressOf());
        }
    }

    if (dxBindFlags & D3D11_BIND_RENDER_TARGET) {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format                   = dxFormat;
        rtvDesc.ViewDimension            = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.ArraySize = 1;
        for (uint32_t face = 0; face < 6; ++face) {
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            entry.CubemapMipRTVs[face].resize(desc.Mips);
            for (uint32_t mip = 0; mip < desc.Mips; ++mip) {
                rtvDesc.Texture2DArray.MipSlice = mip;
                Utils::Check << device->CreateRenderTargetView(entry.Texture.Get(), &rtvDesc, entry.CubemapMipRTVs[face][mip].GetAddressOf());
            }
        }
    }
}

// ─── static members ───────────────────────────────────────────────────────────

ComPtr<ID3D11Device>        SenDx11Backend::_device;
ComPtr<ID3D11DeviceContext> SenDx11Backend::_context;

std::unordered_map<uint32_t, SenDx11TextureEntry> SenDx11Backend::_textures;
uint32_t SenDx11Backend::_nextTextureId = 1;

std::unordered_map<uint32_t, SenDx11BufferEntry> SenDx11Backend::_buffers;
uint32_t SenDx11Backend::_nextBufferId = 1;

std::unordered_map<uint32_t, SenDx11SamplerEntry> SenDx11Backend::_samplers;
uint32_t SenDx11Backend::_nextSamplerId = 1;

std::unordered_map<uint32_t, SenDx11ShaderEntry> SenDx11Backend::_shaders;
uint32_t SenDx11Backend::_nextShaderId = 1;

std::unordered_map<uint32_t, SenDx11PipelineEntry> SenDx11Backend::_pipelines;
uint32_t SenDx11Backend::_nextPipelineId = 1;

std::unordered_map<uint32_t, SenBindGroupLayoutDesc> SenDx11Backend::_bindGroupLayouts;
uint32_t SenDx11Backend::_nextBindGroupLayoutId = 1;

std::unordered_map<uint32_t, SenBindGroupDesc> SenDx11Backend::_bindGroups;
uint32_t SenDx11Backend::_nextBindGroupId = 1;

// ─── initialization ───────────────────────────────────────────────────────────

auto SenDx11Backend::Init(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& context) -> void {
    _device = device;
    _context = context;
}

auto SenDx11Backend::Shutdown() -> void {
    for (auto& pair : _shaders) {
        delete pair.second.SlangBlobPtr;
    }
    _device.Reset();
    _context.Reset();
    _textures.clear();
    _buffers.clear();
    _samplers.clear();
    _shaders.clear();
    _pipelines.clear();
    _bindGroups.clear();
}

// ─── textures ─────────────────────────────────────────────────────────────────

auto SenDx11Backend::CreateTexture(const SenTextureDesc& desc) -> SenTexture {
    const SenTexture handle { _nextTextureId++ };
    auto& entry = _textures[handle.ID];

    if (desc.Cubemap)
        CreateTextureCubemap(_device, desc, entry);
    else
        CreateTexture2D(_device, desc, entry);

    return handle;
}

auto SenDx11Backend::DestroyTexture(SenTexture handle) -> void {
    _textures.erase(handle.ID);
}

auto SenDx11Backend::LookupTexture(SenTexture handle) -> SenDx11TextureEntry& {
    return _textures.at(handle.ID);
}

// ─── buffers ──────────────────────────────────────────────────────────────────

auto SenDx11Backend::CreateBuffer(const SenBufferDesc& desc) -> SenBuffer {
    const SenBuffer handle { _nextBufferId++ };
    auto& entry = _buffers[handle.ID];
    entry.Access = desc.Access;

    const auto     accessDesc = Sen::Dx11::ToBufferAccess(desc.Access);
    const uint32_t bindFlag   = Sen::Dx11::ToBufferBindFlag(desc.Usage);

    // constant buffers must be a multiple of 16 bytes
    uint32_t byteWidth = desc.Size;
    if (desc.Usage == SenBufferUsage::Constant)
        byteWidth = ((byteWidth + 15) / 16) * 16;

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth      = byteWidth;
    bd.Usage          = accessDesc.usage;
    bd.BindFlags      = bindFlag;
    bd.CPUAccessFlags = accessDesc.cpuAccessFlags;

    D3D11_SUBRESOURCE_DATA  initData = {};
    D3D11_SUBRESOURCE_DATA* pInit    = nullptr;
    if (desc.Data) {
        initData.pSysMem = desc.Data;
        pInit = &initData;
    }

    Utils::Check << _device->CreateBuffer(&bd, pInit, entry.Buffer.GetAddressOf());

    return handle;
}

auto SenDx11Backend::DestroyBuffer(SenBuffer handle) -> void {
    _buffers.erase(handle.ID);
}

auto SenDx11Backend::LookupBuffer(SenBuffer handle) -> SenDx11BufferEntry& {
    return _buffers.at(handle.ID);
}

auto SenDx11Backend::WriteBuffer(
    SenBuffer handle,
    const void* data,
    uint32_t size
) -> void {
    auto& entry = _buffers.at(handle.ID);

    if (entry.Access == SenBufferAccess::Dynamic) {
        D3D11_MAPPED_SUBRESOURCE mapped;
        Utils::Check << _context->Map(entry.Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, data, size);
        _context->Unmap(entry.Buffer.Get(), 0);
    }
    else if (entry.Access == SenBufferAccess::Default) {
        _context->UpdateSubresource(entry.Buffer.Get(), 0, nullptr, data, 0, 0);
    }
    else {
        be_assert(false, "SenDx11Backend::WriteBuffer: cannot write to an Immutable buffer");
    }
}

// ─── samplers ─────────────────────────────────────────────────────────────────

auto SenDx11Backend::CreateSampler(const SenSamplerDesc& desc) -> SenSampler {
    const SenSampler handle { _nextSamplerId++ };
    auto& entry = _samplers[handle.ID];

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter         = Sen::Dx11::ToFilter(desc.Filter, desc.Comparison);
    sd.AddressU       = Sen::Dx11::ToAddressMode(desc.Address);
    sd.AddressV       = Sen::Dx11::ToAddressMode(desc.Address);
    sd.AddressW       = Sen::Dx11::ToAddressMode(desc.Address);
    sd.MipLODBias     = 0.f;
    sd.MaxAnisotropy  = (desc.Filter == SenFilter::Anisotropic) ? 16 : 1;
    sd.ComparisonFunc = desc.Comparison ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_NEVER;
    sd.MinLOD         = 0.f;
    sd.MaxLOD         = D3D11_FLOAT32_MAX;

    Utils::Check << _device->CreateSamplerState(&sd, entry.Sampler.GetAddressOf());

    return handle;
}

auto SenDx11Backend::DestroySampler(SenSampler handle) -> void {
    _samplers.erase(handle.ID);
}

auto SenDx11Backend::LookupSampler(SenSampler handle) -> SenDx11SamplerEntry& {
    return _samplers.at(handle.ID);
}

// ─── shaders ──────────────────────────────────────────────────────────────────

auto SenDx11Backend::CreateShader(const SenShaderSourceDesc& sourceDesc) -> SenShader {
    SlangStage slangStage = SLANG_STAGE_NONE;
    switch (sourceDesc.Stage) {
        case SenShaderStage::Vertex: slangStage = SLANG_STAGE_VERTEX; break;
        case SenShaderStage::Hull: slangStage = SLANG_STAGE_HULL; break;
        case SenShaderStage::Domain: slangStage = SLANG_STAGE_DOMAIN; break;
        case SenShaderStage::Pixel: slangStage = SLANG_STAGE_PIXEL; break;
        default: be_assert(false, "SenDx11Backend::CreateShader: unsupported shader stage"); break;
    }

    auto compileResult = SenShaderCompiler::Compile(sourceDesc.SourcePath, sourceDesc.FunctionName, slangStage);
    be_assert(compileResult, "SenDx11Backend::CreateShader: shader compilation failed: " + compileResult.error());

    const SenShader handle { _nextShaderId++ };
    auto& entry = _shaders[handle.ID];

    auto blobPtr = new Slang::ComPtr<ISlangBlob>(compileResult.value());
    entry.SlangBlobPtr = blobPtr;

    const void* bytecodePtr = blobPtr->get()->getBufferPointer();
    size_t bytecodeSize = blobPtr->get()->getBufferSize();

    switch (sourceDesc.Stage) {
        case SenShaderStage::Vertex: {
            ComPtr<ID3D11VertexShader> vs;
            Utils::Check << _device->CreateVertexShader(bytecodePtr, bytecodeSize, nullptr, &vs);
            entry.Shader = vs;
            break;
        }
        case SenShaderStage::Pixel: {
            ComPtr<ID3D11PixelShader> ps;
            Utils::Check << _device->CreatePixelShader(bytecodePtr, bytecodeSize, nullptr, &ps);
            entry.Shader = ps;
            break;
        }
        case SenShaderStage::Hull: {
            ComPtr<ID3D11HullShader> hs;
            Utils::Check << _device->CreateHullShader(bytecodePtr, bytecodeSize, nullptr, &hs);
            entry.Shader = hs;
            break;
        }
        case SenShaderStage::Domain: {
            ComPtr<ID3D11DomainShader> ds;
            Utils::Check << _device->CreateDomainShader(bytecodePtr, bytecodeSize, nullptr, &ds);
            entry.Shader = ds;
            break;
        }
        default:
            be_assert(false, "SenDx11Backend::CreateShader: unsupported shader stage");
    }

    return handle;
}

auto SenDx11Backend::DestroyShader(SenShader handle) -> void {
    delete _shaders[handle.ID].SlangBlobPtr;
    _shaders.erase(handle.ID);
}

auto SenDx11Backend::LookupShader(SenShader handle) -> SenDx11ShaderEntry& {
    return _shaders.at(handle.ID);
}


// ─── pipelines ────────────────────────────────────────────────────────────────

auto SenDx11Backend::CreatePipeline(const SenPipelineDesc& desc) -> SenPipeline {
    const SenPipeline handle { _nextPipelineId++ };
    auto& entry = _pipelines[handle.ID];

    // Extract shader objects from handles
    auto& vsEntry = _shaders.at(desc.VertexShader.ID);
    entry.VertexShader = static_cast<ID3D11VertexShader*>(vsEntry.Shader.Get());

    if (desc.HullShader.IsValid()) {
        auto& hsEntry = _shaders.at(desc.HullShader.ID);
        entry.HullShader = static_cast<ID3D11HullShader*>(hsEntry.Shader.Get());
    }

    if (desc.DomainShader.IsValid()) {
        auto& dsEntry = _shaders.at(desc.DomainShader.ID);
        entry.DomainShader = static_cast<ID3D11DomainShader*>(dsEntry.Shader.Get());
    }

    if (desc.PixelShader.IsValid()) {
        auto& psEntry = _shaders.at(desc.PixelShader.ID);
        entry.PixelShader = static_cast<ID3D11PixelShader*>(psEntry.Shader.Get());
    }

    // Extract input layout (optional for shaders using SV_VertexID, etc.)
    if (!desc.VertexLayout.empty()) {
        std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
        inputLayoutDesc.reserve(desc.VertexLayout.size());

        for (const auto& element : desc.VertexLayout) {
            inputLayoutDesc.push_back({
                .SemanticName         = Sen::Dx11::ToVertexSemanticName(element.Semantic),
                .SemanticIndex        = 0,
                .Format               = Sen::Dx11::ToFormat(element.Format),
                .InputSlot            = 0,
                .AlignedByteOffset    = element.Offset,
                .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            });
        }
        
        be_assert(vsEntry.SlangBlobPtr, "SenDx11Backend::CreatePipeline: vertex shader has no bytecode");
        Utils::Check << _device->CreateInputLayout(
            inputLayoutDesc.data(),
            static_cast<UINT>(inputLayoutDesc.size()),
            vsEntry.SlangBlobPtr->get()->getBufferPointer(),
            vsEntry.SlangBlobPtr->get()->getBufferSize(),
            entry.InputLayout.GetAddressOf()
        );
    }

    // Set topology
    entry.Topology = Sen::Dx11::ToTopology(desc.Topology);

    // Create rasterizer state
    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode              = Sen::Dx11::ToFillMode(desc.RasterizerState.FillMode);
        rd.CullMode              = Sen::Dx11::ToCullMode(desc.RasterizerState.CullMode);
        rd.FrontCounterClockwise = FALSE;
        rd.DepthBias             = static_cast<INT>(desc.RasterizerState.DepthBias);
        rd.DepthBiasClamp        = 0.f;
        rd.SlopeScaledDepthBias  = desc.RasterizerState.SlopeScaledDepthBias;
        rd.DepthClipEnable       = desc.RasterizerState.DepthClipEnable;
        rd.ScissorEnable         = desc.RasterizerState.ScissorEnable;
        rd.MultisampleEnable     = FALSE;
        rd.AntialiasedLineEnable = FALSE;

        Utils::Check << _device->CreateRasterizerState(&rd, entry.RasterizerState.GetAddressOf());
    }

    // Create blend state
    {
        D3D11_BLEND_DESC bd = {};
        bd.AlphaToCoverageEnable  = FALSE;
        bd.IndependentBlendEnable = FALSE;

        D3D11_RENDER_TARGET_BLEND_DESC& rtbd = bd.RenderTarget[0];
        rtbd.BlendEnable    = desc.BlendState.Enable;
        rtbd.SrcBlend       = Sen::Dx11::ToBlendFactor(desc.BlendState.SrcBlend);
        rtbd.DestBlend      = Sen::Dx11::ToBlendFactor(desc.BlendState.DstBlend);
        rtbd.BlendOp        = Sen::Dx11::ToBlendOp(desc.BlendState.BlendOp);
        rtbd.SrcBlendAlpha  = Sen::Dx11::ToBlendFactor(desc.BlendState.SrcBlendAlpha);
        rtbd.DestBlendAlpha = Sen::Dx11::ToBlendFactor(desc.BlendState.DstBlendAlpha);
        rtbd.BlendOpAlpha   = Sen::Dx11::ToBlendOp(desc.BlendState.BlendOpAlpha);
        rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        Utils::Check << _device->CreateBlendState(&bd, entry.BlendState.GetAddressOf());
    }

    // Create depth-stencil state
    {
        D3D11_DEPTH_STENCIL_DESC dsd = {};
        dsd.DepthEnable    = desc.DepthStencilState.DepthEnable;
        dsd.DepthWriteMask = desc.DepthStencilState.DepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        dsd.DepthFunc      = Sen::Dx11::ToComparisonFunc(desc.DepthStencilState.DepthFunc);
        dsd.StencilEnable  = FALSE;

        Utils::Check << _device->CreateDepthStencilState(&dsd, entry.DepthStencilState.GetAddressOf());
    }

    return handle;
}

auto SenDx11Backend::DestroyPipeline(SenPipeline handle) -> void {
    _pipelines.erase(handle.ID);
}

auto SenDx11Backend::LookupPipeline(SenPipeline handle) -> SenDx11PipelineEntry& {
    return _pipelines.at(handle.ID);
}

// ─── render passes ────────────────────────────────────────────────────────────

auto SenDx11Backend::RegisterBackbuffer(const ComPtr<ID3D11RenderTargetView>& backbufferRTV) -> SenTexture {
    const SenTexture handle { _nextTextureId++ };
    auto& entry = _textures[handle.ID];
    entry.MipRTVs.resize(1);
    entry.MipRTVs[0] = backbufferRTV;
    // Don't set Texture, SRV, DSV, CubemapDSVs, CubemapMipRTVs (backbuffer is output-only)
    return handle;
}


// ─── bind group layouts ───────────────────────────────────────────────────────

auto SenDx11Backend::CreateBindGroupLayout(const SenBindGroupLayoutDesc& desc) -> SenBindGroupLayout {
    const SenBindGroupLayout handle { _nextBindGroupLayoutId++ };
    _bindGroupLayouts[handle.ID] = desc;
    return handle;
}

auto SenDx11Backend::DestroyBindGroupLayout(SenBindGroupLayout handle) -> void {
    _bindGroupLayouts.erase(handle.ID);
}

auto SenDx11Backend::LookupBindGroupLayout(SenBindGroupLayout handle) -> SenBindGroupLayoutDesc& {
    return _bindGroupLayouts.at(handle.ID);
}


// ─── bind groups ──────────────────────────────────────────────────────────────

auto SenDx11Backend::CreateBindGroup(const SenBindGroupDesc& desc) -> SenBindGroup {
    const SenBindGroup handle { _nextBindGroupId++ };
    _bindGroups[handle.ID] = desc;
    return handle;
}

auto SenDx11Backend::DestroyBindGroup(SenBindGroup handle) -> void {
    _bindGroups.erase(handle.ID);
}

auto SenDx11Backend::LookupBindGroup(SenBindGroup handle) -> SenBindGroupDesc& {
    return _bindGroups.at(handle.ID);
}

