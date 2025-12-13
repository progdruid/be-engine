#pragma once
#include <memory>
#include <umbrellas/access-modifiers.hpp>

#include "BeMaterial.h"
#include "BeShader.h"


class BePipeline {

    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    static auto Create (const ComPtr<ID3D11DeviceContext>& context) -> std::shared_ptr<BePipeline>;

    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    ComPtr<ID3D11DeviceContext> _context;
    
    BeShaderType _boundShaderType = BeShaderType::None;
    std::shared_ptr<BeShader> _boundShader;
    std::shared_ptr<BeMaterial> _boundMaterial;

    
    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide BePipeline() = default;
    expose ~BePipeline() = default;

    
    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto GetRawContext () -> ComPtr<ID3D11DeviceContext> { return _context; }
    
    expose
    auto BindShader (const std::shared_ptr<BeShader>& shader, BeShaderType shaderType) -> void;
    auto BindMaterial (const std::shared_ptr<BeMaterial>& material) -> void;
    auto Clear() -> void;

};
