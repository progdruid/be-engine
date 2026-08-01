#pragma once
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <umbrellas/common.hpp>

#include "BeShaderTools.h"
#include "sen-rhi/SenTypes.h"

struct BeMaterialPropertyDescriptor {
    enum class Type : uint8_t {
        Float,
        Float2,
        Float3,
        Float4,
        Matrix
    };

    std::string Name;
    Type PropertyType;
    uint32_t ArrayLength = 1;
    std::vector<float> DefaultValue;
};

struct BeMaterialTextureDescriptor {
    std::string Name;
    uint8_t SlotIndex;
    std::string DefaultTexturePath;
    bool IsStorage = false;
};

struct BeMaterialSamplerDescriptor {
    std::string Name;
    uint8_t SlotIndex;
    std::string DefaultSamplerDescString;
};

class BeMaterialScheme {
    expose static auto Create (
        const std::string& name,
        const std::vector<BeShaderTools::ParsedMaterialProperty>& properties
    ) -> BeMaterialScheme;
    
    expose
    std::string Name;
    std::vector<BeMaterialPropertyDescriptor> Properties;
    std::vector<BeMaterialTextureDescriptor> Textures;
    std::vector<BeMaterialSamplerDescriptor> Samplers;
    SenBindGroupDesc BindGroupLayout;

    std::unordered_map<std::string, uint32_t> PropertyOffsets;      // in floats
    std::unordered_map<std::string, uint32_t> PropertyArrayLengths;
    uint32_t CbufferSize = 0;                                       // in bytes, padded
};
