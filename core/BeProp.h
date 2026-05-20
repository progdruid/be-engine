#pragma once
#include <memory>
#include <vector>

#include "BeMesh.h"

class BeShader;
class BeMaterial;

struct BePropSlice {
    std::shared_ptr<BeMaterial> Material;
    bool TwoSided = false;
};

struct BeProp {
    std::shared_ptr<BeMesh> Mesh;
    std::vector<BePropSlice> Slices;
    std::shared_ptr<BeShader> Shader;
    std::vector<std::shared_ptr<BeMaterial>> Materials;

    static auto FromMesh(
        std::shared_ptr<BeMesh> mesh,
        std::weak_ptr<BeShader> shader,
        const std::string& schemeName
    ) -> std::shared_ptr<BeProp>;

    BeProp() = default;
    ~BeProp() = default;
};
