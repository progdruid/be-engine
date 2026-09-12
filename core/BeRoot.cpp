#include "BeRoot.h"

#include <cstring>
#include <umbrellas/include-libassert.h>

#include "BeMaterial.h"
#include "BeShader.h"

BeRoot::BeRoot(const BeShader& shader) : _layout(&shader.GetRootLayout()) {
    be_assert(
        _layout->Size <= SenMaxRootConstantSize, 
        "BeRoot: root exceeds push-constant capacity", 
        _layout->Size
    );
    _written.assign(_layout->Fields.size(), false);
}

auto BeRoot::Use(const std::string& link, BeMaterial& material) -> BeRoot& {
    for (size_t i = 0; i < _layout->Fields.size(); ++i) {
        const auto& field = _layout->Fields[i];
        if (field.Link != link) {
            continue;
        }

        switch (field.Kind) {
            case BeShaderTools::RootFieldKind::Pointer: {
                const uint64_t address = material.GetCbufferAddress().Value;
                std::memcpy(_data.data() + field.Offset, &address, sizeof(address));
                break;
            }
            case BeShaderTools::RootFieldKind::TextureIndex: {
                const uint32_t index = material.GetTextureHeapIndex(field.PropertyName);
                std::memcpy(_data.data() + field.Offset, &index, sizeof(index));
                break;
            }
            case BeShaderTools::RootFieldKind::SamplerIndex: {
                const uint32_t index = material.GetSamplerHeapIndex(field.PropertyName);
                std::memcpy(_data.data() + field.Offset, &index, sizeof(index));
                break;
            }
        }
        _written[i] = true;
    }
    return *this;
}

auto BeRoot::Push(SenCommandBuffer& cmd) -> void {
    if (_layout->Size == 0) {
        return;
    }
    for (size_t i = 0; i < _written.size(); ++i) {
        be_assert(
            _written[i],
            "BeRoot: field not filled before Push",
            _layout->Fields[i].FieldName
        );
    }
    cmd.PushRoot(_data.data(), _layout->Size);
}
