#pragma once
#include <exception>
#include <iostream>
#include <string>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <windows.h>
#include <type_traits>
#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>

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

    struct ErrorStream {
        ErrorStream& operator<<(const HRESULT msg) {
            if (FAILED(msg)) {
                std::cerr << "Error: HRESULT 0x" << std::hex << msg << std::dec << "\n";
                throw com_exception(msg);
            }
            return *this;
        }
    };

    inline ErrorStream Check;


    inline ID3D11Buffer* NullBuffers            [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    inline ID3D11SamplerState* NullSamplers     [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};
    inline ID3D11ShaderResourceView* NullSRVs   [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    inline ID3D11RenderTargetView* NullRTVs     [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

    // ========== Debug Annotation Helpers for RenderDoc/PIX ==========

    /**
     * BeDebugAnnotation provides RenderDoc/PIX debug marker support
     * Usage:
     *   {
     *       BeDebugAnnotation marker(context, "Pass Name");
     *       // ... rendering code ...
     *   } // marker automatically ends on scope exit
     */
    class BeDebugAnnotation {
    public:
        explicit BeDebugAnnotation(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, const std::string& label)
            : _context(context) {
            if (!_context) return;

            // Query for user-defined annotation interface
            _context->QueryInterface(IID_PPV_ARGS(&_annotation));

            if (_annotation) {
                // Convert string to wide string for DirectX API
                std::wstring wideLabel(label.begin(), label.end());
                _annotation->BeginEvent(wideLabel.c_str());
            }
        }

        ~BeDebugAnnotation() {
            if (_annotation) {
                _annotation->EndEvent();
                // ComPtr automatically releases on destruction
            }
        }

        // Deleted copy/move operations - markers should be scoped
        BeDebugAnnotation(const BeDebugAnnotation&) = delete;
        BeDebugAnnotation& operator=(const BeDebugAnnotation&) = delete;
        BeDebugAnnotation(BeDebugAnnotation&&) = delete;
        BeDebugAnnotation& operator=(BeDebugAnnotation&&) = delete;

    private:
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context;
        Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> _annotation;
    };

}


template<typename E>
struct EnableBitmaskOperators : std::false_type {};

#define ENABLE_BITMASK(E) \
template<> struct EnableBitmaskOperators<E> : std::true_type {}

template<class BitmaskEnumType>
constexpr auto operator|(BitmaskEnumType a, BitmaskEnumType b) -> BitmaskEnumType requires EnableBitmaskOperators<BitmaskEnumType>::value {
    using UnderlyingUIntType = std::underlying_type_t<BitmaskEnumType>;
    return static_cast<BitmaskEnumType>(static_cast<UnderlyingUIntType>(a) | static_cast<UnderlyingUIntType>(b));
}

template<class BitmaskEnumType>
constexpr auto operator&(BitmaskEnumType a, BitmaskEnumType b) -> BitmaskEnumType requires EnableBitmaskOperators<BitmaskEnumType>::value {
    using UnderlyingUIntType = std::underlying_type_t<BitmaskEnumType>;
    return static_cast<BitmaskEnumType>(static_cast<UnderlyingUIntType>(a) & static_cast<UnderlyingUIntType>(b));
}

template<class BitmaskEnumType>
constexpr auto operator^(BitmaskEnumType a, BitmaskEnumType b) -> BitmaskEnumType requires EnableBitmaskOperators<BitmaskEnumType>::value {
    using UnderlyingUIntType = std::underlying_type_t<BitmaskEnumType>;
    return static_cast<BitmaskEnumType>(static_cast<UnderlyingUIntType>(a) ^ static_cast<UnderlyingUIntType>(b));
}

template<class BitmaskEnumType>
constexpr auto operator~(BitmaskEnumType a) -> BitmaskEnumType requires EnableBitmaskOperators<BitmaskEnumType>::value {
    using UnderlyingUIntType = std::underlying_type_t<BitmaskEnumType>;
    return static_cast<BitmaskEnumType>(~static_cast<UnderlyingUIntType>(a));
}

template<class BitmaskEnumType>
constexpr auto operator|=(BitmaskEnumType a, BitmaskEnumType b) -> BitmaskEnumType requires EnableBitmaskOperators<BitmaskEnumType>::value {
    return a = (a | b);
}

template<class BitmaskEnumType>
constexpr auto operator&=(BitmaskEnumType a, BitmaskEnumType b) -> BitmaskEnumType requires EnableBitmaskOperators<BitmaskEnumType>::value {
    return a = (a & b);
}

template<class BitmaskEnumType>
constexpr auto operator^=(BitmaskEnumType a, BitmaskEnumType b) -> BitmaskEnumType requires EnableBitmaskOperators<BitmaskEnumType>::value {
    return a = (a ^ b);
}

// Helpers
template<class BitmaskEnumType>
constexpr auto HasAny(BitmaskEnumType value, BitmaskEnumType mask) -> bool requires EnableBitmaskOperators<BitmaskEnumType>::value {
    using UnderlyingUIntType = std::underlying_type_t<BitmaskEnumType>;
    return (static_cast<UnderlyingUIntType>(value) & static_cast<UnderlyingUIntType>(mask)) != 0;
}

template<class BitmaskEnumType>
constexpr auto HasAll(BitmaskEnumType value, BitmaskEnumType mask) -> bool requires EnableBitmaskOperators<BitmaskEnumType>::value {
    using UnderlyingUIntType = std::underlying_type_t<BitmaskEnumType>;
    return (static_cast<UnderlyingUIntType>(value) & static_cast<UnderlyingUIntType>(mask)) == static_cast<UnderlyingUIntType>(mask);
}
