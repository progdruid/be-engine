#pragma once
#include <exception>
#include <iostream>
#include <string>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <windows.h>

namespace Utils {
  
    struct com_exception : public std::exception {
    private:
        HRESULT _hr;

    public:
        explicit com_exception(const HRESULT hr) : _hr(hr) {}

        const char* what() const noexcept override {
            static char str[64] = {};
            snprintf(str, sizeof(str), "Failure with HRESULT of 0x%08X", static_cast<unsigned int>(_hr));
            return str;
        }


    };

    inline auto ThrowIfFailed(const HRESULT hr) -> void {
        if (FAILED(hr))
            throw com_exception(hr);
    }

    inline auto PrintMaterialInfo(const aiMaterial* material) -> void {
        aiString name;
        if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            std::cout << "Material Name: " << name.C_Str() << "\n";
        }
        aiColor3D color;
        if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS) {
            std::cout << "Base Color: " << color.r << ", " << color.g << ", " << color.b << "\n";
        }
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            std::cout << "Diffuse Color: " << color.r << ", " << color.g << ", " << color.b << "\n";
        }
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            std::cout << "Specular Color: " << color.r << ", " << color.g << ", " << color.b << "\n";
        }
        float shininess, strength;
        if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS &&
            material->Get(AI_MATKEY_SHININESS_STRENGTH, strength) == AI_SUCCESS) {
            std::cout << "Shininess: " << shininess << ", Strength: " << strength << "\n";
        }
        int twoSided;
        if (material->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS) {
            std::cout << "Two-sided: " << (twoSided ? "true" : "false") << "\n";
        }
        float opacity;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
            std::cout << "Opacity: " << opacity << "\n";
        }
        
        // Print texture info for each texture type
        for (int type = aiTextureType_NONE; type <= aiTextureType_UNKNOWN; ++type) {
            int texCount = material->GetTextureCount((aiTextureType)type);
            for (int i = 0; i < texCount; ++i) {
                aiString texPath;
                if (material->GetTexture((aiTextureType)type, i, &texPath) == AI_SUCCESS) {
                    std::cout << "Texture " << i << " Type " << type << ": " << texPath.C_Str() << "\n";
                }
            }
        }
    }

    inline auto PrintNode(const aiNode* node, const aiScene* scene, int depth = 0) -> void {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "Node " << node->mName.C_Str() << " with " << node->mNumMeshes << " mesh(es)\n";

        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            unsigned int meshIndex = node->mMeshes[i];
            const aiMesh* mesh = scene->mMeshes[meshIndex];
            for (int j = 0; j < depth + 1; ++j) std::cout << "  ";
            std::cout << "Mesh " << meshIndex << " with " << mesh->mNumVertices << " vertices, Material: " << mesh->mMaterialIndex << "\n";
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            PrintNode(node->mChildren[i], scene, depth + 1);
        }
    }

    inline void PrintSceneInfo(const aiScene* scene) {
        std::cout << "---- Scene Information ----\n";
        std::cout << "Meshes: " << scene->mNumMeshes << "\n";
        std::cout << "Materials: " << scene->mNumMaterials << "\n";
        std::cout << "Textures: " << scene->mNumTextures << "\n";
        std::cout << "Animations: " << scene->mNumAnimations << "\n";
        std::cout << "Lights: " << scene->mNumLights << "\n";
        std::cout << "Cameras: " << scene->mNumCameras << "\n";

        // Print materials
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            std::cout << "\nMaterial " << i << ":\n";
            PrintMaterialInfo(scene->mMaterials[i]);
        }

        // Print node hierarchy
        std::cout << "\nScene Nodes:\n";
        PrintNode(scene->mRootNode, scene);
    }
}
