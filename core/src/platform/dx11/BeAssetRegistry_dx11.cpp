#include "BeAssetRegistry.h"
#include "BeRendererImpl.h"
#include "DxUtils.h"

#include "BeRenderer.h"
#include "BeShaderTools.h"

struct BeSamplerImpl {
    ComPtr<ID3D11SamplerState> samplerState;
};

auto BeAssetRegistry::GetSampler(std::string_view samplerDescString) -> BeSampler {
    auto key = std::string(samplerDescString);

    if (_samplers.contains(key)) {
        return _samplers[key];
    }

    auto tokens = BeShaderTools::Split(samplerDescString, "-");
    be_assert(
        tokens.size() == 2 || tokens.size() == 3,
        "Invalid samplerDescString. Expected format: filter-address[-cmp]",
        samplerDescString,
        tokens.size()
    );

    auto filterToken = std::string(tokens[0]);
    auto addressToken = std::string(tokens[1]);
    auto hasComparison = tokens.size() == 3 && tokens[2] == "cmp";

    D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    if (filterToken == "point") {
        filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    } else if (filterToken == "linear") {
        filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    } else {
        be_assert(false, "Unknown filter token", filterToken);
    }

    D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_CLAMP;
    if (addressToken == "wrap") {
        addressMode = D3D11_TEXTURE_ADDRESS_WRAP;
    } else if (addressToken == "clamp") {
        addressMode = D3D11_TEXTURE_ADDRESS_CLAMP;
    } else if (addressToken == "mirror") {
        addressMode = D3D11_TEXTURE_ADDRESS_MIRROR;
    } else {
        be_assert(false, "Unknown address token", addressToken);
    }

    if (hasComparison) {
        if (filter == D3D11_FILTER_MIN_MAG_MIP_POINT)
            filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        else if (filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR)
            filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        else if (filter == D3D11_FILTER_ANISOTROPIC)
            filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
    }

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = filter;
    samplerDesc.AddressU = addressMode;
    samplerDesc.AddressV = addressMode;
    samplerDesc.AddressW = addressMode;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = hasComparison ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    auto renderer = _renderer.lock();
    be_assert(renderer, "Renderer couldn't be locked");

    auto device = renderer->GetPlatformImpl()->device;
    auto samplerState = std::make_shared<BeSamplerImpl>();

    DxUtils::Check << device->CreateSamplerState(&samplerDesc, &samplerState->samplerState);

    _samplers[key] = samplerState;
    return samplerState;
}
