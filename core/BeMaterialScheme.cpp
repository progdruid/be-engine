#include "BeMaterialScheme.h"

#include "sen-rhi/SenBackend.h"


auto BeMaterialScheme::CreateFromJson(
    const std::string& name, 
    const Json& json
) -> BeMaterialScheme {
    
    auto materialScheme = BeMaterialScheme();
    materialScheme.Name = name;
    
    for (const auto& propertyItemJson : json) {
        auto parsedProperty = BeShaderTools::ParseMaterialProperty(propertyItemJson);
    
        // extracting
        if (parsedProperty.Type == "texture2d" || parsedProperty.Type == "textureCube") {
            auto descriptor = BeMaterialTextureDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.DefaultTexturePath = parsedProperty.Default;
            materialScheme.Textures.push_back(descriptor);
        }
        else if (parsedProperty.Type == "sampler") {
            auto descriptor = BeMaterialSamplerDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.DefaultSamplerDescString = parsedProperty.Default;
            materialScheme.Samplers.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float") {
            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float;
            descriptor.DefaultValue.push_back(std::stof(parsedProperty.Default));
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float2") {
            auto json = Json::parse(parsedProperty.Default, nullptr, true, true, true);
            auto vec = json.get<std::vector<float>>();
            assert(vec.size() == 2);
        
            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float2;
            descriptor.DefaultValue = vec;
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float3") {
            auto json = Json::parse(parsedProperty.Default, nullptr, true, true, true);
            auto vec = json.get<std::vector<float>>();
            assert(vec.size() == 3);
        
            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float3;
            descriptor.DefaultValue = vec;
            materialScheme.Properties.push_back(descriptor);
        }
        else if (parsedProperty.Type == "float4") {
            Json j = Json::parse(parsedProperty.Default, nullptr, true, true, true);
            const auto vec = j.get<std::vector<float>>();
            assert(vec.size() == 4);
        
            auto descriptor = BeMaterialPropertyDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float4;
            descriptor.DefaultValue = vec;
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
    
    SenBindGroupDesc desc = {};
    desc.Stages = SenShaderStageFlags::AllGraphics;
    
    if (!materialScheme.Properties.empty()) {
        desc.BufferSlots = { 0 };
    }
    
    uint8_t textureSlotsStart = 1 + materialScheme.Samplers.size();
    if (!materialScheme.Samplers.empty()) {
        auto samplerRange = std::views::iota(uint8_t(1), textureSlotsStart);
        desc.SamplerSlots = std::vector<uint8_t>(samplerRange.begin(), samplerRange.end());
        for (size_t i = 0; i < materialScheme.Samplers.size(); ++i) {
            materialScheme.Samplers[i].SlotIndex = 1 + uint8_t(i);
        }
    }

    if (!materialScheme.Textures.empty()) {
        uint8_t textureSlotsEnd = textureSlotsStart + materialScheme.Textures.size();
        auto textureRange = std::views::iota(textureSlotsStart, textureSlotsEnd);
        desc.TextureSlots = std::vector<uint8_t>(textureRange.begin(), textureRange.end());
        for (size_t i = 0; i < materialScheme.Textures.size(); ++i) {
            materialScheme.Textures[i].SlotIndex = textureSlotsStart + uint8_t(i);
        }
    }
    
    materialScheme.BindGroupLayout = desc;
    return materialScheme;
}
