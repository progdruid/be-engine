#include "SenDx11CommandBuffer.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/dx11/SenDx11Backend.h>

SenDx11CommandBuffer::SenDx11CommandBuffer(const ComPtr<ID3D11DeviceContext>& context)
    : _context(context)
{}


// ─── render pass ──────────────────────────────────────────────────────────────

auto SenDx11CommandBuffer::BeginPass(const SenPassDesc& desc) -> void {
    // Collect RTVs for color attachments
    std::vector<ID3D11RenderTargetView*> rtvs;
    rtvs.reserve(desc.ColorAttachments.size());

    for (const auto& attachment : desc.ColorAttachments) {
        auto& texEntry = SenDx11Backend::LookupTexture(attachment.Texture);

        ID3D11RenderTargetView* rtv = nullptr;
        if (attachment.CubemapFace >= 0) {
            be_assert(attachment.CubemapFace < 6, "SenDx11CommandBuffer::BeginPass: invalid cubemap face");
            rtv = texEntry.CubemapMipRTVs[attachment.CubemapFace][attachment.MipLevel].Get();
        } else {
            rtv = texEntry.MipRTVs[attachment.MipLevel].Get();
        }

        if (attachment.LoadOp == SenLoadOp::Clear) {
            _context->ClearRenderTargetView(rtv, glm::value_ptr(attachment.ClearColor));
        }

        rtvs.push_back(rtv);
    }

    // Get DSV for depth attachment
    ID3D11DepthStencilView* dsv = nullptr;
    if (desc.DepthAttachment.has_value()) {
        const auto& depthAttach = desc.DepthAttachment.value();
        auto& texEntry = SenDx11Backend::LookupTexture(depthAttach.Texture);

        if (depthAttach.CubemapFace >= 0) {
            be_assert(depthAttach.CubemapFace < 6, "SenDx11CommandBuffer::BeginPass: invalid cubemap face for depth");
            dsv = texEntry.CubemapDSVs[depthAttach.CubemapFace].Get();
        } else {
            dsv = texEntry.DSV.Get();
        }

        if (depthAttach.LoadOp == SenLoadOp::Clear) {
            _context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depthAttach.ClearDepth, depthAttach.ClearStencil);
        }
    }

    // Bind render targets
    _context->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.data(), dsv);

    // Set viewport
    be_assert(
        desc.Viewport.Width > 0.f && desc.Viewport.Height > 0.f,
        "SenDx11CommandBuffer::BeginPass: viewport width and height must be positive"
    );
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = desc.Viewport.X;
    vp.TopLeftY = desc.Viewport.Y;
    vp.Width    = desc.Viewport.Width;
    vp.Height   = desc.Viewport.Height;
    vp.MinDepth = desc.Viewport.MinDepth;
    vp.MaxDepth = desc.Viewport.MaxDepth;
    _context->RSSetViewports(1, &vp);
}

auto SenDx11CommandBuffer::EndPass() -> void {
    // DX11 immediate mode: nothing to do (no explicit render pass end)
}


// ─── pipeline + resources ─────────────────────────────────────────────────────

auto SenDx11CommandBuffer::SetPipeline(SenPipeline pipeline) -> void {
    auto& entry = SenDx11Backend::LookupPipeline(pipeline);

    _boundVertexShader = entry.VertexShader;
    _boundHullShader   = entry.HullShader;
    _boundDomainShader = entry.DomainShader;
    _boundPixelShader  = entry.PixelShader;

    _context->IASetPrimitiveTopology(entry.Topology);

    if (entry.InputLayout) {
        _context->IASetInputLayout(entry.InputLayout.Get());
    }
    if (entry.VertexShader) {
        _context->VSSetShader(entry.VertexShader.Get(), nullptr, 0);
    }
    if (entry.HullShader) {
        _context->HSSetShader(entry.HullShader.Get(), nullptr, 0);
    }
    if (entry.DomainShader) {
        _context->DSSetShader(entry.DomainShader.Get(), nullptr, 0);
    }
    if (entry.PixelShader) {
        _context->PSSetShader(entry.PixelShader.Get(), nullptr, 0);
    }

    float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    _context->RSSetState(entry.RasterizerState.Get());
    _context->OMSetBlendState(entry.BlendState.Get(), blendFactor, 0xffffffff);
    _context->OMSetDepthStencilState(entry.DepthStencilState.Get(), 0);
}

auto SenDx11CommandBuffer::SetBindGroup(SenBindGroup group, uint8_t index) -> void {
    const auto& desc = SenDx11Backend::LookupBindGroup(group);
    const auto& layout = SenDx11Backend::LookupBindGroupLayout(desc.Layout);
    
    be_assert(desc.Textures.size() == layout.TextureEntries.size());
    be_assert(desc.Samplers.size() == layout.SamplerEntries.size());
    be_assert(desc.ConstantBuffers.size() == layout.BufferEntries.size());
    
    for (size_t i = 0; i < desc.Textures.size(); ++i) {
        auto* srv = SenDx11Backend::LookupTexture(desc.Textures[i]).SRV.Get();
        auto slot = layout.TextureEntries[i].Slot;
        if (_boundVertexShader) {
            _context->VSSetShaderResources(slot, 1, &srv);
        }
        if (_boundHullShader && _boundDomainShader) {
            _context->HSSetShaderResources(slot, 1, &srv);
            _context->DSSetShaderResources(slot, 1, &srv);
        }
        if (_boundPixelShader) {
            _context->PSSetShaderResources(slot, 1, &srv);
        }
    }

    for (size_t i = 0; i < desc.Samplers.size(); ++i) {
        auto* sampler = SenDx11Backend::LookupSampler(desc.Samplers[i]).Sampler.Get();
        auto slot = layout.SamplerEntries[i].Slot;
        if (_boundVertexShader) {
            _context->VSSetSamplers(slot, 1, &sampler);
        }
        if (_boundHullShader && _boundDomainShader) {
            _context->HSSetSamplers(slot, 1, &sampler);
            _context->DSSetSamplers(slot, 1, &sampler);
        }
        if (_boundPixelShader) {
            _context->PSSetSamplers(slot, 1, &sampler);
        }
    }

    for (size_t i = 0; i < desc.ConstantBuffers.size(); ++i) {
        auto* buffer = SenDx11Backend::LookupBuffer(desc.ConstantBuffers[i]).Buffer.Get();
        auto slot = layout.BufferEntries[i].Slot;
        if (_boundVertexShader) {
            _context->VSSetConstantBuffers(slot, 1, &buffer);
        }
        if (_boundHullShader && _boundDomainShader) {
            _context->HSSetConstantBuffers(slot, 1, &buffer);
            _context->DSSetConstantBuffers(slot, 1, &buffer);
        }
        if (_boundPixelShader) {
            _context->PSSetConstantBuffers(slot, 1, &buffer);
        }
    }
}

auto SenDx11CommandBuffer::SetVertexBuffer(SenBuffer buffer, uint32_t stride) -> void {
    auto* raw = SenDx11Backend::LookupBuffer(buffer).Buffer.Get();
    const UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, &raw, &stride, &offset);
}

auto SenDx11CommandBuffer::SetIndexBuffer(SenBuffer buffer) -> void {
    auto* raw = SenDx11Backend::LookupBuffer(buffer).Buffer.Get();
    _context->IASetIndexBuffer(raw, DXGI_FORMAT_R32_UINT, 0);
}

auto SenDx11CommandBuffer::ClearVertexBuffer() -> void {
    ID3D11Buffer* null = nullptr;
    const UINT zero = 0;
    _context->IASetVertexBuffers(0, 1, &null, &zero, &zero);
}

auto SenDx11CommandBuffer::ClearIndexBuffer() -> void {
    _context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
}

auto SenDx11CommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex) -> void {
    _context->Draw(vertexCount, firstVertex);
}

auto SenDx11CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void {
    _context->DrawIndexed(indexCount, firstIndex, baseVertex);
}
