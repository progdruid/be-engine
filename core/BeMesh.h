#pragma once
#include <cstdint>
#include <vector>
#include <include-glm.h>

struct BeFullVertex {
    glm::vec3 Position;                 // 0
    glm::vec3 Normal;                   // 12
    glm::vec4 Color     {1, 1, 1, 1};   // 24
    glm::vec2 UV0       {0, 0};         // 40
    glm::vec4 Tangent   {1, 0, 0, 1};   // 48  xyz=tangent, w=handedness
};

struct BeSubmesh {
    uint32_t IndexCount;
    uint32_t StartIndexLocation;
    int32_t  BaseVertexLocation;
};

struct BeMeshData {
    std::vector<BeFullVertex> Vertices;
    std::vector<uint32_t>     Indices;
    std::vector<BeSubmesh>    Submeshes;
};

struct BeMesh {
    uint32_t Index = 0;
    uint32_t Generation = 0;
    auto IsValid() const -> bool { return Generation != 0; }
    auto operator==(const BeMesh& other) const -> bool = default;
};
