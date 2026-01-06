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
    expose static auto CreateFromJson (
        const std::string& name, 
        const Json& json
    ) -> BeMaterialScheme;
    
    expose 
    std::string Name;
    std::vector<BeMaterialPropertyDescriptor> Properties;
    std::vector<BeMaterialTextureDescriptor> Textures;
    std::vector<BeMaterialSamplerDescriptor> Samplers;
};
