#pragma once
#include <memory>
#include <vector>

#include "BeDrawState.h"
#include "BeMesh.h"
#include <umbrellas/common.hpp>

struct BeShader;
class BeMaterial;

struct BePropSlice {
    std::shared_ptr<BeMaterial> Material;
    bool TwoSided = false;
};

struct BeProp {
    std::shared_ptr<BeMesh> Mesh;
    std::vector<BePropSlice> Slices;
    BeDrawState State;
    std::vector<std::shared_ptr<BeMaterial>> Materials;

    static auto FromMesh(
        std::shared_ptr<BeMesh> mesh,
        raw_ptr<BeShader> shader,
        const std::string& schemeLink
    ) -> std::shared_ptr<BeProp>;

    BeProp() = default;
    ~BeProp() = default;
};
