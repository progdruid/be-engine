#include "BeMaterialBinding.h"

#include <umbrellas/include-libassert.h>
#include "BeMaterial.h"
#include "BeShader.h"
#include "sen-rhi/SenBackend.h"

BeMaterialBinding::~BeMaterialBinding() {
    if (_bindGroup.IsValid()) {
        SenBackend::DestroyBindGroup(_bindGroup);
    }
}

BeMaterialBinding::BeMaterialBinding(BeMaterialBinding&& other) noexcept
    : _material      (std::move(other._material))
    , _shader        (std::move(other._shader))
    , _bindGroup     (other._bindGroup)
    , _cachedVersion (other._cachedVersion)
{
    other._bindGroup = {};
}

BeMaterialBinding& BeMaterialBinding::operator=(BeMaterialBinding&& other) noexcept {
    if (this == &other) { return *this; }
    if (_bindGroup.IsValid()) {
        SenBackend::DestroyBindGroup(_bindGroup);
    }
    _material      = std::move(other._material);
    _shader        = std::move(other._shader);
    _bindGroup     = other._bindGroup;
    _cachedVersion = other._cachedVersion;
    other._bindGroup = {};
    return *this;
}

auto BeMaterialBinding::Make(std::shared_ptr<BeMaterial> material, std::weak_ptr<BeShader> shader) -> void {
    _material = std::move(material);
    _shader   = std::move(shader);
    Resolve();
}

auto BeMaterialBinding::Resolve() -> SenBindGroup {
    be_assert(_material, "BeMaterialBinding::Resolve called on empty binding");

    _material->FlushBuffer();

    if (_cachedVersion != _material->GetVersion()) {
        if (_bindGroup.IsValid()) {
            SenBackend::DestroyBindGroup(_bindGroup);
        }

        auto shader = _shader.lock();
        be_assert(shader, "BeMaterialBinding::Resolve — shader has expired");

        const uint8_t cbufferSlot = shader->GetMaterialSlotByScheme(_material->GetSchemeName());
        _bindGroup     = SenBackend::CreateBindGroup(_material->BuildBindGroupDesc(cbufferSlot));
        _cachedVersion = _material->GetVersion();
    }

    return _bindGroup;
}

auto BeMaterialBinding::GetMaterial() const -> std::weak_ptr<BeMaterial> {
    return _material;
}
