#pragma once
#include <memory>
#include <umbrellas/access-modifiers.hpp>
#include "sen-rhi/SenTypes.h"

class BeMaterial;
class BeShader;

class BeMaterialBinding {
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    std::shared_ptr<BeMaterial> _material;
    std::weak_ptr<BeShader>     _shader;
    SenBindGroup                _bindGroup;
    uint32_t                    _cachedVersion = 0;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    BeMaterialBinding() = default;
    ~BeMaterialBinding();

    BeMaterialBinding(const BeMaterialBinding&)            = delete;
    BeMaterialBinding& operator=(const BeMaterialBinding&) = delete;
    BeMaterialBinding(BeMaterialBinding&&) noexcept;
    BeMaterialBinding& operator=(BeMaterialBinding&&) noexcept;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto Make (std::shared_ptr<BeMaterial> material, std::weak_ptr<BeShader> shader) -> void;
    auto Resolve () -> SenBindGroup;
    auto GetMaterial () const -> std::weak_ptr<BeMaterial> ;
    //auto IsValid     () const -> bool { return _material != nullptr; } // not sure whether we need this
};
