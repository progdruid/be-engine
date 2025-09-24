#include "BeModel.h"

#include <glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Utils.h"

BeModel::BeModel(const std::filesystem::path& modelPath, ID3D11Device* device) {

    Assimp::Importer importer;
    constexpr auto flags = (
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_CalcTangentSpace |
        aiProcess_ValidateDataStructure |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph);

    const aiScene* scene = importer.ReadFile(modelPath.string().c_str(), flags);
    if (!scene || !scene->mRootNode)
        throw std::runtime_error("Failed to load model: " + modelPath.string());
    
    Utils::PrintSceneInfo(scene);


    
    MeshInstructions.reserve(scene->mNumMeshes);
    
    size_t numVertices = 0;
    size_t numIndices = 0;
    for (unsigned i=0; i<scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        numVertices += mesh->mNumVertices;
        numIndices += 3 * mesh->mNumFaces;
    }

    FullVertices.reserve(numVertices); 
    Indices.reserve(numIndices);
    
    int32_t vertexOffset = 0; // indexing in each mesh starts from 0
    uint32_t indexOffset = 0;
    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        
        for (size_t v = 0; v < mesh->mNumVertices; ++v) {
            BeFullVertex vertex{};
            aiVector3D position = mesh->mVertices[v];
            aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.f, 1.f, 0.f);

            //temporary
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiColor4D color;
            aiColor3D matColor;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, matColor) == AI_SUCCESS) {
                color = aiColor4D(matColor.r, matColor.g, matColor.b, 1.f);
            }
            aiVector3D texCoord0 = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][v] : aiVector3D(0.f, 0.f, 0.f);
            aiVector3D texCoord1 = mesh->HasTextureCoords(1) ? mesh->mTextureCoords[1][v] : aiVector3D(0.f, 0.f, 0.f);
            aiVector3D texCoord2 = mesh->HasTextureCoords(2) ? mesh->mTextureCoords[2][v] : aiVector3D(0.f, 0.f, 0.f);
            vertex.Position = {-position.x, position.y, position.z};
            vertex.Normal = {-normal.x, normal.y, normal.z};
            vertex.Color = {color.r, color.g, color.b, color.a};
            vertex.UV0 = {texCoord0.x, texCoord0.y};
            vertex.UV1 = {texCoord1.x, texCoord1.y};
            vertex.UV2 = {texCoord2.x, texCoord2.y};
            FullVertices.push_back(vertex);
        }
        
        for (size_t f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            Indices.push_back(face.mIndices[0]);
            Indices.push_back(face.mIndices[2]);
            Indices.push_back(face.mIndices[1]);
        }

        auto material = scene->mMaterials[mesh->mMaterialIndex];
        ComPtr<ID3D11ShaderResourceView> srv = nullptr;
        aiString texPath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            if (texPath.C_Str()[0] != '*') {
                throw std::runtime_error("Only embedded textures are supported for now");
            } // use stb_image
            
            char* endPtr;
            const long texIndex = std::strtol(texPath.C_Str() + 1, &endPtr, 10);
            const aiTexture* aiTex = scene->mTextures[texIndex];

            // handle compressed texture
            DecodedImage decodedImage{};
            if (!ToRGBA8(aiTex, decodedImage)) {
                throw std::runtime_error("Failed to decompress texture");
            }
            FlipImageVertically(decodedImage);

            D3D11_TEXTURE2D_DESC desc = {
                .Width = decodedImage.Width,
                .Height = decodedImage.Height,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = { .Count = 1 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = 0, 
            };

            D3D11_SUBRESOURCE_DATA initData = {
                .pSysMem = decodedImage.Pixels,
                .SysMemPitch = 4 * decodedImage.Width
            };
            
            ComPtr<ID3D11Texture2D> texture = nullptr;
            Utils::ThrowIfFailed(device->CreateTexture2D(&desc, &initData, &texture));
            
            // Create SRV
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDescriptor = {
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
                .Texture2D = { .MostDetailedMip = 0, .MipLevels = 1 },
            };
            Utils::ThrowIfFailed(device->CreateShaderResourceView(texture.Get(), &srvDescriptor, srv.GetAddressOf()));
            free(decodedImage.Pixels); // free the decoded image memory
        }

        
        MeshInstructions.push_back({
            .IndexCount = mesh->mNumFaces * 3,
            .StartIndexLocation = indexOffset,
            .BaseVertexLocation = vertexOffset,
            .DiffuseTexture = srv //temporary
        });
        
        vertexOffset += mesh->mNumVertices;
        indexOffset += mesh->mNumFaces * 3;
    }
}

auto BeModel::ToRGBA8(const aiTexture* texture, DecodedImage& out) -> bool {
    if (!texture) return false;

    // Case 1: compressed data (PNG/JPG/etc.). aiTexture stores raw file bytes in pcData.
    if (texture->mHeight == 0) {
        // mWidth = data length in bytes; pcData is a byte blob despite aiTexel type.
        const auto data = reinterpret_cast<const uint8_t*>(texture->pcData);
        const int dataSize = static_cast<int>(texture->mWidth);
        int w = 0, h = 0, channelsInFile = 0;

        // Force 4 channels to standardize output layout as RGBA8.
        uint8_t* decoded = stbi_load_from_memory(data, dataSize, &w, &h, &channelsInFile, 4);
        if (!decoded) return false;

        // Allocate new space for the decoded image using your own allocator (malloc here)
        const size_t imageSize = static_cast<size_t>(w) * static_cast<size_t>(h) * 4; // RGBA8
        const auto pixels = static_cast<uint8_t*>(malloc(imageSize));
        if (!pixels) {
            stbi_image_free(decoded);
            return false;
        }
        memcpy(pixels, decoded, imageSize);
        stbi_image_free(decoded);

        out.Pixels = pixels; // free with free()
        out.Width = w;
        out.Height = h;
        return true;
    }

    // Case 2: uncompressed data; pcData is aiTexel array of RGBA 8-bit.
    // mWidth, mHeight are pixel dimensions; total texels = mWidth * mHeight.
    {
        const size_t count = static_cast<size_t>(texture->mWidth) * static_cast<size_t>(texture->mHeight);
        if (count == 0) return false;

        // aiTexel is 4 bytes (rgba). Copy as RGBA8.
        const auto pixels = static_cast<uint8_t*>(malloc(count * 4));
        if (!pixels) return false;

        const aiTexel* src = texture->pcData;
        for (size_t i = 0; i < count; ++i) {
            pixels[4 * i + 0] = src[i].r;
            pixels[4 * i + 1] = src[i].g;
            pixels[4 * i + 2] = src[i].b;
            pixels[4 * i + 3] = src[i].a;
        }

        out.Pixels = pixels;     // free with free()
        out.Width = static_cast<int>(texture->mWidth);
        out.Height = static_cast<int>(texture->mHeight);
        return true;
    }


}

auto BeModel::FlipImageVertically(DecodedImage& image) -> void {
    const uint32_t rowSize = image.Width * 4; // 4 bytes per pixel (RGBA8)
    uint8_t* tempRow = new uint8_t[rowSize];

    for (uint32_t y = 0; y < image.Height / 2; ++y) {
        uint8_t* topRow = image.Pixels + y * rowSize;
        uint8_t* bottomRow = image.Pixels + (image.Height - 1 - y) * rowSize;

        // Swap topRow and bottomRow
        memcpy(tempRow, topRow, rowSize);
        memcpy(topRow, bottomRow, rowSize);
        memcpy(bottomRow, tempRow, rowSize);
    }

    delete[] tempRow;
}
