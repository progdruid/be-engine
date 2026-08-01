#include "BeMaterialScheme.h"

#include <ranges>

#include "sen-rhi/SenBackend.h"
#include "umbrellas/include-libassert.h"

// std140 layout, matches Vulkan UBOs, WebGPU/WGSL, and Slang's HLSL output.
static const std::unordered_map<BeMaterialPropertyDescriptor::Type, uint32_t> SizeMap = {
    {BeMaterialPropertyDescriptor::Type::Float,  uint32_t(1 * sizeof(float))},
    {BeMaterialPropertyDescriptor::Type::Float2, uint32_t(2 * sizeof(float))},
    {BeMaterialPropertyDescriptor::Type::Float3, uint32_t(3 * sizeof(float))},
    {BeMaterialPropertyDescriptor::Type::Float4, uint32_t(4 * sizeof(float))},
    {BeMaterialPropertyDescriptor::Type::Matrix, uint32_t(16 * sizeof(float))},
};
static const std::unordered_map<BeMaterialPropertyDescriptor::Type, uint32_t> AlignMap = {
    {BeMaterialPropertyDescriptor::Type::Float,  4},
    {BeMaterialPropertyDescriptor::Type::Float2, 8},
    {BeMaterialPropertyDescriptor::Type::Float3, 16},
    {BeMaterialPropertyDescriptor::Type::Float4, 16},
    {BeMaterialPropertyDescriptor::Type::Matrix, 16},
};

// std140 array rule: every element starts on a 16-byte boundary, so the stride is 16 bytes.
static constexpr uint32_t ArrayElementStride = 16;


auto BeMaterialScheme::Create(
    const std::string& name,
    const std::vector<BeShaderTools::ParsedMaterialProperty>& properties
) -> BeMaterialScheme {

    auto materialScheme = BeMaterialScheme();
    materialScheme.Name = name;

    for (const auto& parsedProperty : properties) {

        // extracting
        if (parsedProperty.Type == "texture2d" || parsedProperty.Type == "textureCube") {
            auto descriptor = BeMaterialTextureDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.DefaultTexturePath = parsedProperty.Default;
            materialScheme.Textures.push_back(descriptor);
        }
        else if (parsedProperty.Type == "storage texture2d") {
            auto descriptor = BeMaterialTextureDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.DefaultTexturePath = parsedProperty.Default;
            descriptor.IsStorage = true;
            materialScheme.Textures.push_back(descriptor);
        }
        else if (parsedProperty.Type == "sampler") {
            auto descriptor = BeMaterialSamplerDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.DefaultSamplerDescString = parsedProperty.Default;
            materialScheme.Samplers.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float" && parsedProperty.ArrayLength == 1) {
            auto value = BeShaderTools::ParseFloat(parsedProperty.Default);
            be_assert(value.has_value(), parsedProperty.Name + " -> " + (value ? std::string() : value.error()));

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float;
            descriptor.DefaultValue = { *value };
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float2" && parsedProperty.ArrayLength == 1) {
            auto vec = BeShaderTools::ParseTuple(parsedProperty.Default);
            be_assert(vec.has_value(), parsedProperty.Name + " -> " + (vec ? std::string() : vec.error()));
            be_assert(vec->size() == 2, parsedProperty.Name);

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float2;
            descriptor.DefaultValue = std::move(*vec);
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float3" && parsedProperty.ArrayLength == 1) {
            auto vec = BeShaderTools::ParseTuple(parsedProperty.Default);
            be_assert(vec.has_value(), parsedProperty.Name + " -> " + (vec ? std::string() : vec.error()));
            be_assert(vec->size() == 3, parsedProperty.Name);

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float3;
            descriptor.DefaultValue = std::move(*vec);
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float4" && parsedProperty.ArrayLength == 1) {
            auto vec = BeShaderTools::ParseTuple(parsedProperty.Default);
            be_assert(vec.has_value(), parsedProperty.Name + " -> " + (vec ? std::string() : vec.error()));
            be_assert(vec->size() == 4, parsedProperty.Name);

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float4;
            descriptor.DefaultValue = std::move(*vec);
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float" && parsedProperty.ArrayLength > 1) {
            auto vec = BeShaderTools::ParseFloatArray(parsedProperty.Default, 1);
            be_assert(vec.has_value(), parsedProperty.Name + " -> " + (vec ? std::string() : vec.error()));
            be_assert(vec->size() <= size_t(parsedProperty.ArrayLength) * 1, parsedProperty.Name + ": too many array default entries");
            vec->resize(size_t(parsedProperty.ArrayLength) * 1, 0.0f);

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float;
            descriptor.ArrayLength = parsedProperty.ArrayLength;
            descriptor.DefaultValue = std::move(*vec);
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float2" && parsedProperty.ArrayLength > 1) {
            auto vec = BeShaderTools::ParseFloatArray(parsedProperty.Default, 2);
            be_assert(vec.has_value(), parsedProperty.Name + " -> " + (vec ? std::string() : vec.error()));
            be_assert(vec->size() <= size_t(parsedProperty.ArrayLength) * 2, parsedProperty.Name + ": too many array default entries");
            vec->resize(size_t(parsedProperty.ArrayLength) * 2, 0.0f);

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float2;
            descriptor.ArrayLength = parsedProperty.ArrayLength;
            descriptor.DefaultValue = std::move(*vec);
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float3" && parsedProperty.ArrayLength > 1) {
            auto vec = BeShaderTools::ParseFloatArray(parsedProperty.Default, 3);
            be_assert(vec.has_value(), parsedProperty.Name + " -> " + (vec ? std::string() : vec.error()));
            be_assert(vec->size() <= size_t(parsedProperty.ArrayLength) * 3, parsedProperty.Name + ": too many array default entries");
            vec->resize(size_t(parsedProperty.ArrayLength) * 3, 0.0f);

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float3;
            descriptor.ArrayLength = parsedProperty.ArrayLength;
            descriptor.DefaultValue = std::move(*vec);
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float4" && parsedProperty.ArrayLength > 1) {
            auto vec = BeShaderTools::ParseFloatArray(parsedProperty.Default, 4);
            be_assert(vec.has_value(), parsedProperty.Name + " -> " + (vec ? std::string() : vec.error()));
            be_assert(vec->size() <= size_t(parsedProperty.ArrayLength) * 4, parsedProperty.Name + ": too many array default entries");
            vec->resize(size_t(parsedProperty.ArrayLength) * 4, 0.0f);

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float4;
            descriptor.ArrayLength = parsedProperty.ArrayLength;
            descriptor.DefaultValue = std::move(*vec);
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "matrix") {
            std::vector<float> mat = {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        
            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Matrix;
            descriptor.DefaultValue = mat;
            materialScheme.Properties.push_back(descriptor);
        }
    }
    
    uint32_t offsetBytes = 0;
    for (const auto& property : materialScheme.Properties) {
        const bool isArray = property.ArrayLength > 1;
        const uint32_t size  = isArray ? ArrayElementStride * property.ArrayLength : SizeMap.at(property.PropertyType);
        const uint32_t align = isArray ? ArrayElementStride : AlignMap.at(property.PropertyType);

        offsetBytes = (offsetBytes + align - 1) / align * align;

        materialScheme.PropertyOffsets[property.Name] = offsetBytes / sizeof(float);
        materialScheme.PropertyArrayLengths[property.Name] = property.ArrayLength;
        offsetBytes += size;
    }
    materialScheme.CbufferSize = (offsetBytes + 15) / 16 * 16;

    SenBindGroupDesc desc = {};
    desc.Stages = SenShaderStageFlags::AllGraphics | SenShaderStageFlags::Compute;

    if (!materialScheme.Properties.empty()) {
        desc.BufferSlots = { 0 };
        desc.BufferDynamicRanges = { materialScheme.CbufferSize };
    }

    uint8_t textureSlotsStart = 1 + materialScheme.Samplers.size();
    if (!materialScheme.Samplers.empty()) {
        auto samplerRange = std::views::iota(uint8_t(1), textureSlotsStart);
        desc.SamplerSlots = std::vector<uint8_t>(samplerRange.begin(), samplerRange.end());
        for (size_t i = 0; i < materialScheme.Samplers.size(); ++i) {
            materialScheme.Samplers[i].SlotIndex = 1 + uint8_t(i);
        }
    }

    uint8_t nextSlot = textureSlotsStart;
    for (auto& texture : materialScheme.Textures) {
        texture.SlotIndex = nextSlot;
        if (texture.IsStorage) {
            desc.StorageTextureSlots.push_back(nextSlot);
        } else {
            desc.TextureSlots.push_back(nextSlot);
        }
        ++nextSlot;
    }

    materialScheme.BindGroupLayout = desc;
    return materialScheme;
}
