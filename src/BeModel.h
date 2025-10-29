#pragma once
#include <filesystem>
#include <glm.hpp>
#include <assimp/scene.h>

class BeTexture;

struct BeFullVertex {
    glm::vec3 Position;                 // 0
    glm::vec3 Normal;                   // 12
    glm::vec4 Color     {1, 1, 1, 1};   // 24
    glm::vec2 UV0       {0, 0};         // 40
    glm::vec2 UV1       {0, 0};         // 48
    glm::vec2 UV2       {0, 0};         // 56
};

struct BeMaterial {
    std::shared_ptr<BeTexture> DiffuseTexture = nullptr;
    std::shared_ptr<BeTexture> SpecularTexture = nullptr;
    
    glm::vec3 DiffuseColor  {1, 1, 1};
    glm::vec3 SpecularColor {1, 1, 1};
    float Shininess = -1.f; 
    glm::vec3 SuperSpecularColor {1, 1, 1};
    float SuperShininess = -1.f; 
};

struct BeModel {
    
    struct BeDrawSlice {
        uint32_t IndexCount;
        uint32_t StartIndexLocation;
        int32_t BaseVertexLocation;
        BeMaterial Material;
    };

    BeModel() = default;
    ~BeModel() = default;

    std::vector<BeDrawSlice> DrawSlices;
    std::vector<BeFullVertex> FullVertices;
    std::vector<uint32_t> Indices;
};
