#pragma once
#include <cstdint>
#include <vector>
#include <umbrellas/include-glm.h>

struct BeFullVertex {
    glm::vec3 Position;                 // 0
    glm::vec3 Normal;                   // 12
    glm::vec4 Color     {1, 1, 1, 1};   // 24
    glm::vec2 UV0       {0, 0};         // 40
    glm::vec4 Tangent   {1, 0, 0, 1};   // 48  xyz=tangent, w=handedness
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
