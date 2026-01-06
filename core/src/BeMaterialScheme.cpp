#include "BeMaterialScheme.h"


auto BeMaterialScheme::CreateFromJson(
    const std::string& name, 
    const Json& json
) -> BeMaterialScheme {
    
    auto materialScheme = BeMaterialScheme();
    materialScheme.Name = name;
    
    for (const auto& propertyItemJson : json) {
        auto parsedProperty = BeShaderTools::ParseMaterialProperty(propertyItemJson);
    
        // extracting
        if (parsedProperty.Type == "texture2d") {
            auto descriptor = BeMaterialTextureDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.SlotIndex = parsedProperty.Slot;
            descriptor.DefaultTexturePath = parsedProperty.Default;
            materialScheme.Textures.push_back(descriptor);
        }
        else if (parsedProperty.Type == "sampler") {
            auto descriptor = BeMaterialSamplerDescriptor();
            descriptor.Name = parsedProperty.Name;
            descriptor.SlotIndex = parsedProperty.Slot;
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
    
    return materialScheme;
}
