#pragma once
#include <cstdint>
#include <vector>
#include <umbrellas/include-glm.h>

struct BeFullVertex {
    glm::vec3 Position;                 // 0
    glm::vec3 Normal;                   // 12
    glm::vec4 Color     {1, 1, 1, 1};   // 24
    glm::vec2 UV0       {0, 0};         // 40
    glm::vec2 UV1       {0, 0};         // 48
    glm::vec2 UV2       {0, 0};         // 56
};

struct BeMeshSlice {
    uint32_t IndexCount;
    uint32_t StartIndexLocation;
    int32_t  BaseVertexLocation;
};

struct BeMesh {
    std::vector<BeFullVertex> Vertices;
    std::vector<uint32_t>     Indices;
    std::vector<BeMeshSlice>  Slices;
};
