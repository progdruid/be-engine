#pragma once
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>
#include <umbrellas/access-modifiers.hpp>

#include "BeShaderTools.h"


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
    std::vector<float> DefaultValue;
};

struct BeMaterialTextureDescriptor {
    std::string Name;
    uint8_t SlotIndex;
    std::string DefaultTexturePath;
};

struct BeMaterialSamplerDescriptor {
    std::string Name;
    uint8_t SlotIndex;
};

class BeMaterialScheme {
    expose static auto Create (
        const std::string& schemeName,
        const std::filesystem::path& schemePath
    ) -> BeMaterialScheme;
    expose static auto Create (
        const std::string& name, 
        const Json& json
    ) -> BeMaterialScheme;
    
    expose 
    std::string Name;
    std::filesystem::path Path;
    std::vector<BeMaterialPropertyDescriptor> Properties;
    std::vector<BeMaterialTextureDescriptor> Textures;
    std::vector<BeMaterialSamplerDescriptor> Samplers;
};
