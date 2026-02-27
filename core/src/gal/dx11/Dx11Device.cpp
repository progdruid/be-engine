#include "Dx11Device.h"
#include "Dx11Buffer.h"
#include "Dx11Sampler.h"
#include "../GalFormatConverter.h"

auto Dx11Device::CreateBuffer(const GalBufferDesc& desc, const void* initialData) -> std::shared_ptr<IGalBuffer> {
    D3D11_BUFFER_DESC d3dDesc = {};
    d3dDesc.ByteWidth = desc.ByteWidth;
    d3dDesc.BindFlags = GalFormatConverter::ToD3DBindFlags(desc.BindFlags);

    switch (desc.Usage) {
        case GalBufferUsage::Dynamic:
            d3dDesc.Usage = D3D11_USAGE_DYNAMIC;
            break;
        case GalBufferUsage::Immutable:
            d3dDesc.Usage = D3D11_USAGE_IMMUTABLE;
            break;
        case GalBufferUsage::Staging:
            d3dDesc.Usage = D3D11_USAGE_STAGING;
            break;
        default:
            d3dDesc.Usage = D3D11_USAGE_DEFAULT;
            break;
    }

    if (desc.CPUWriteAccess)
        d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA subData = {};
    D3D11_SUBRESOURCE_DATA* pSubData = nullptr;
    if (initialData) {
        subData.pSysMem = initialData;
        pSubData = &subData;
    }

    ComPtr<ID3D11Buffer> buffer;
    HRESULT hr = _device->CreateBuffer(&d3dDesc, pSubData, &buffer);
    if (FAILED(hr)) return nullptr;

    return std::make_shared<Dx11Buffer>(buffer, desc.ByteWidth, desc.Usage, desc.BindFlags);
}

auto Dx11Device::CreateSampler(const GalSamplerDesc& desc) -> std::shared_ptr<IGalSampler> {
    D3D11_SAMPLER_DESC d3dDesc = {};

    if (desc.UseComparison) {
        switch (desc.Filter) {
            case GalFilter::Point:       d3dDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT; break;
            case GalFilter::Anisotropic: d3dDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC; break;
            default:                     d3dDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; break;
        }
    } else {
        switch (desc.Filter) {
            case GalFilter::Point:       d3dDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; break;
            case GalFilter::Anisotropic: d3dDesc.Filter = D3D11_FILTER_ANISOTROPIC; break;
            default:                     d3dDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; break;
        }
    }

    auto toAddress = [](GalAddressMode mode) -> D3D11_TEXTURE_ADDRESS_MODE {
        switch (mode) {
            case GalAddressMode::Clamp:  return D3D11_TEXTURE_ADDRESS_CLAMP;
            case GalAddressMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
            case GalAddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
            default:                     return D3D11_TEXTURE_ADDRESS_WRAP;
        }
    };

    d3dDesc.AddressU = toAddress(desc.AddressU);
    d3dDesc.AddressV = toAddress(desc.AddressV);
    d3dDesc.AddressW = toAddress(desc.AddressW);
    d3dDesc.MaxAnisotropy = desc.MaxAnisotropy;
    d3dDesc.MipLODBias = desc.MipLODBias;
    d3dDesc.MinLOD = desc.MinLOD;
    d3dDesc.MaxLOD = desc.MaxLOD;

    auto toComparison = [](GalComparisonFunc func) -> D3D11_COMPARISON_FUNC {
        switch (func) {
            case GalComparisonFunc::Never:        return D3D11_COMPARISON_NEVER;
            case GalComparisonFunc::Less:         return D3D11_COMPARISON_LESS;
            case GalComparisonFunc::Equal:        return D3D11_COMPARISON_EQUAL;
            case GalComparisonFunc::LessEqual:    return D3D11_COMPARISON_LESS_EQUAL;
            case GalComparisonFunc::Greater:      return D3D11_COMPARISON_GREATER;
            case GalComparisonFunc::NotEqual:     return D3D11_COMPARISON_NOT_EQUAL;
            case GalComparisonFunc::GreaterEqual: return D3D11_COMPARISON_GREATER_EQUAL;
            case GalComparisonFunc::Always:       return D3D11_COMPARISON_ALWAYS;
            default:                              return D3D11_COMPARISON_NEVER;
        }
    };

    d3dDesc.ComparisonFunc = toComparison(desc.ComparisonFunc);

    ComPtr<ID3D11SamplerState> sampler;
    HRESULT hr = _device->CreateSamplerState(&d3dDesc, &sampler);
    if (FAILED(hr)) return nullptr;

    return std::make_shared<Dx11Sampler>(sampler);
}
