#include "BePipeline.h"

#include "sen-rhi/SenBackend.h"

auto BePipeline::Create(const ComPtr<ID3D11DeviceContext>& context) -> std::shared_ptr<BePipeline> {
    auto pipeline = std::shared_ptr<BePipeline>(new BePipeline());
    pipeline->_context = context;
    return pipeline;
}

auto BePipeline::BindPipeline(SenPipeline pipeline) -> void {
    auto& entry = SenBackend::LookupPipeline(pipeline);

    _boundVertexShader = entry.VertexShader;
    _boundHullShader   = entry.HullShader;
    _boundDomainShader = entry.DomainShader;
    _boundPixelShader  = entry.PixelShader;

    _context->IASetPrimitiveTopology(entry.Topology);

    // vertex layout is optional for shaders using SV_VertexID
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

auto BePipeline::SetBindGroup(SenBindGroup group, uint8_t slot) -> void {
    const auto& desc = SenBackend::LookupBindGroup(group);

    for (const auto& entry : desc.Textures) {
        auto* srv = SenBackend::LookupTexture(entry.Texture).SRV.Get();
        if (_boundVertexShader) {
            _context->VSSetShaderResources(entry.Slot, 1, &srv);
        }
        if (_boundHullShader && _boundDomainShader) {
            _context->HSSetShaderResources(entry.Slot, 1, &srv);
            _context->DSSetShaderResources(entry.Slot, 1, &srv);
        }
        if (_boundPixelShader) {
            _context->PSSetShaderResources(entry.Slot, 1, &srv);
        }
    }

    for (const auto& entry : desc.Samplers) {
        auto* sampler = SenBackend::LookupSampler(entry.Sampler).Sampler.Get();
        if (_boundVertexShader) {
            _context->VSSetSamplers(entry.Slot, 1, &sampler);
        }
        if (_boundHullShader && _boundDomainShader) {
            _context->HSSetSamplers(entry.Slot, 1, &sampler);
            _context->DSSetSamplers(entry.Slot, 1, &sampler);
        }
        if (_boundPixelShader) {
            _context->PSSetSamplers(entry.Slot, 1, &sampler);
        }
    }

    for (const auto& entry : desc.ConstantBuffers) {
        auto* buffer = SenBackend::LookupBuffer(entry.Buffer).Buffer.Get();
        if (_boundVertexShader) {
            _context->VSSetConstantBuffers(entry.Slot, 1, &buffer);
        }
        if (_boundHullShader && _boundDomainShader) {
            _context->HSSetConstantBuffers(entry.Slot, 1, &buffer);
            _context->DSSetConstantBuffers(entry.Slot, 1, &buffer);
        }
        if (_boundPixelShader) {
            _context->PSSetConstantBuffers(entry.Slot, 1, &buffer);
        }
    }
}

auto BePipeline::BindVertexBuffer(SenBuffer buffer, uint32_t stride) const -> void {
    auto* raw = SenBackend::LookupBuffer(buffer).Buffer.Get();
    const UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, &raw, &stride, &offset);
}

auto BePipeline::BindIndexBuffer(SenBuffer buffer) const -> void {
    auto* raw = SenBackend::LookupBuffer(buffer).Buffer.Get();
    _context->IASetIndexBuffer(raw, DXGI_FORMAT_R32_UINT, 0);
}

auto BePipeline::ClearVertexBuffer() const -> void {
    ID3D11Buffer* null = nullptr;
    const UINT zero = 0;
    _context->IASetVertexBuffers(0, 1, &null, &zero, &zero);
}

auto BePipeline::ClearIndexBuffer() const -> void {
    _context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
}

auto BePipeline::Draw(uint32_t vertexCount, uint32_t startVertexLocation) const -> void {
    _context->Draw(vertexCount, startVertexLocation);
}

auto BePipeline::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) const -> void {
    _context->DrawIndexed(indexCount, startIndex, baseVertex);
}
