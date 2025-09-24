#pragma once
#include <assimp/material.h>

class BeMaterial {
public:
    explicit BeMaterial(const aiMaterial* assimpMaterial);
    ~BeMaterial() = default;

    uint32_t DiffuseTexture;
};
