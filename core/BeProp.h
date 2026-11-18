#pragma once
#include <memory>
#include <vector>

#include "BeMesh.h"

class BeMaterial;

struct BePropSurface {
    static auto AllocateId() -> uint32_t;

    uint32_t Id = AllocateId();
    uint32_t Submesh = 0;
    std::shared_ptr<BeMaterial> Material;
};

struct BeProp {
    std::shared_ptr<BeMeshData> MeshData;
    BeMesh Mesh;
    std::vector<BePropSurface> Surfaces;

    BeProp() = default;
    ~BeProp();
    BeProp(const BeProp&) = delete;
    auto operator=(const BeProp&) -> BeProp& = delete;
};
