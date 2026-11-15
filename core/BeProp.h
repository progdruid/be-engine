#pragma once
#include <memory>
#include <vector>

#include "BeDrawState.h"
#include "BeMesh.h"
#include <common.hpp>

struct BeShader;
class BeMaterial;

struct BePropSlice {
    std::shared_ptr<BeMaterial> Material;
    bool TwoSided = false;
};

// todo: twosided is just a cull mode. it constitutes BeDrawState.
//  ideally this needs a drawstate per slice? perhaps? i am uncertain

// todo: BeProp and BeMesh should get a system instead of shared_ptrs

// todo: reorganise prop slices
//  
// continuing my thoughts on organising props
// what is needed to render something?
// its four things: 
// 1. targets where to render to, 
// 2. pipeline state, aka how to render, aka BeDrawState
// 3. geometry, aka what to render
// 4. additional data for rendering, aka BeMaterials
// 
// hence i am thinking that a propslice can have 2-4: geometry slice (mesh slice), draw state per slice and materials per slice
// and BeProp then becomes a collection of prop slices
//
// i am also thinking that mesh slices should have address or handle of their gpu buffer / mesh
// then a mesh slice will be a comprehensive geometry pointer


struct BeProp {
    std::shared_ptr<BeMesh> Mesh;
    std::vector<BePropSlice> Slices; 
    BeDrawState DrawState; 
    std::vector<std::shared_ptr<BeMaterial>> Materials;

    static auto FromMesh(
        std::shared_ptr<BeMesh> mesh,
        raw_ptr<BeShader> shader,
        const std::string& schemeLink
    ) -> std::shared_ptr<BeProp>;

    BeProp() = default;
    ~BeProp() = default;
};
