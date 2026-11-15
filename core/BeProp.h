#pragma once
#include <memory>
#include <vector>

#include "BeMesh.h"

class BeMaterial;

struct BePropSurface {
    static auto AllocateId() -> uint32_t;

    uint32_t Id = AllocateId();
    uint32_t MeshSlice = 0;
    std::shared_ptr<BeMaterial> Material;
};

// todo: BeProp and BeMesh should get a system instead of shared_ptrs
//  i am also thinking that mesh slices should have address or handle of their gpu buffer / mesh
//  then a mesh slice will be a comprehensive geometry pointer


struct BeProp {
    std::shared_ptr<BeMesh> Mesh;
    std::vector<BePropSurface> Surfaces;

    BeProp() = default;
    ~BeProp() = default;
};
