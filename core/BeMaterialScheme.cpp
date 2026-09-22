#include "BeMaterialScheme.h"

#include <ranges>

#include "Sen.h"
#include "include-libassert.h"

// Scalar (natural) layout, matching Slang's layout for BDA pointer-backed cbuffers.
// Every member aligns to its scalar component (4 bytes); arrays are tightly packed.
static const std::unordered_map<BeMaterialPropertyDescriptor::Type, uint32_t> SizeMap = {
    {BeMaterialPropertyDescriptor::Type::Float,  uint32_t(1 * sizeof(float))},
    {BeMaterialPropertyDescriptor::Type::Float2, uint32_t(2 * sizeof(float))},
    {BeMaterialPropertyDescriptor::Type::Float3, uint32_t(3 * sizeof(float))},
    {BeMaterialPropertyDescriptor::Type::Float4, uint32_t(4 * sizeof(float))},
    {BeMaterialPropertyDescriptor::Type::Matrix, uint32_t(16 * sizeof(float))},
};
static const std::unordered_map<BeMaterialPropertyDescriptor::Type, uint32_t> AlignMap = {
    {BeMaterialPropertyDescriptor::Type::Float,  4},
    {BeMaterialPropertyDescriptor::Type::Float2, 4},
    {BeMaterialPropertyDescriptor::Type::Float3, 4},
    {BeMaterialPropertyDescriptor::Type::Float4, 4},
    {BeMaterialPropertyDescriptor::Type::Matrix, 4},
};


auto BeMaterialScheme::Create(
    const std::string& name,
    const std::vector<BeShaderTools::ParsedMaterialProperty>& properties
) -> BeMaterialScheme {

    auto materialScheme = BeMaterialScheme();
    materialScheme.Name = name;

    for (const auto& parsedProperty : properties) {

        // extracting
        if (parsedProperty.Type == "texture2d"   || parsedProperty.Type == "textureCube"
         || parsedProperty.Type == "texture2d[]" || parsedProperty.Type == "textureCube[]") {
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
        else if (parsedProperty.Type == "sampler" || parsedProperty.Type == "comparison sampler") {
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
        else if (parsedProperty.Type == "matrix" && parsedProperty.ArrayLength == 1) {
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
        else if (parsedProperty.Type == "matrix" && parsedProperty.ArrayLength > 1) {
            static constexpr float identity[16] = {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
            std::vector<float> mats;
            mats.reserve(size_t(parsedProperty.ArrayLength) * 16);
            for (uint32_t k = 0; k < parsedProperty.ArrayLength; ++k) {
                mats.insert(mats.end(), identity, identity + 16);
            }

            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Matrix;
            descriptor.ArrayLength = parsedProperty.ArrayLength;
            descriptor.DefaultValue = std::move(mats);
            materialScheme.Properties.push_back(descriptor);
        }
    }
    
    uint32_t offsetBytes = 0;
    for (const auto& property : materialScheme.Properties) {
        const bool isArray = property.ArrayLength > 1;
        // Scalar layout: array elements are tightly packed at their natural size.
        const uint32_t elementStride = SizeMap.at(property.PropertyType);
        const uint32_t size  = isArray ? elementStride * property.ArrayLength : SizeMap.at(property.PropertyType);
        const uint32_t align = AlignMap.at(property.PropertyType);

        offsetBytes = (offsetBytes + align - 1) / align * align;

        materialScheme.PropertyOffsets[property.Name] = offsetBytes / sizeof(float);
        materialScheme.PropertyArrayLengths[property.Name] = property.ArrayLength;
        offsetBytes += size;
    }
    materialScheme.CbufferSize = (offsetBytes + 15) / 16 * 16;

    return materialScheme;
}
