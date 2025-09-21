#pragma once
#include <filesystem>
#include <glm.hpp>

class ModelAsset {
    struct DrawInstruction {
        uint32_t IndexCount;
        uint32_t StartIndexLocation;
        int32_t BaseVertexLocation;
    };
    
public:
    explicit ModelAsset(const std::filesystem::path& filePath);
    ~ModelAsset() = default;

public:
    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec3> Colors;
    std::vector<uint32_t> Indices;
    std::vector<DrawInstruction> DrawInstructions;
    
};
