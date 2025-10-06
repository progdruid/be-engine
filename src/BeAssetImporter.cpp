#include "BeAssetImporter.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Utils.h"

BeAssetImporter::BeAssetImporter(const ComPtr<ID3D11Device>& device) {
    _device = device;
}

auto BeAssetImporter::LoadModel(const std::filesystem::path& modelPath) -> std::shared_ptr<BeModel> {
    constexpr auto flags = (
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_CalcTangentSpace |
        aiProcess_ValidateDataStructure |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph);

    const aiScene* scene = _importer.ReadFile(modelPath.string().c_str(), flags);
    if (!scene || !scene->mRootNode)
        throw std::runtime_error("Failed to load model: " + modelPath.string());
    
    Utils::PrintSceneInfo(scene);


    auto model = std::make_shared<BeModel>();
    model->DrawSlices.reserve(scene->mNumMeshes);

    size_t numVertices = 0;
    size_t numIndices = 0;
    for (unsigned i=0; i<scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        numVertices += mesh->mNumVertices;
        numIndices += 3 * mesh->mNumFaces;
    }

    model->FullVertices.reserve(numVertices); 
    model->Indices.reserve(numIndices);
    
    int32_t vertexOffset = 0; // indexing in each mesh starts from 0
    uint32_t indexOffset = 0;
    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        
        for (size_t v = 0; v < mesh->mNumVertices; ++v) {
            BeFullVertex vertex{};
            aiVector3D position = mesh->mVertices[v];
            aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.f, 1.f, 0.f);
            aiColor4D color = mesh->HasVertexColors(0) ? mesh->mColors[0][v] : aiColor4D(1, 1, 1, 1);
            aiVector3D texCoord0 = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][v] : aiVector3D(0.f, 0.f, 0.f);
            aiVector3D texCoord1 = mesh->HasTextureCoords(1) ? mesh->mTextureCoords[1][v] : aiVector3D(0.f, 0.f, 0.f);
            aiVector3D texCoord2 = mesh->HasTextureCoords(2) ? mesh->mTextureCoords[2][v] : aiVector3D(0.f, 0.f, 0.f);
            vertex.Position = {-position.x, position.y, position.z};
            vertex.Normal = {-normal.x, normal.y, normal.z};
            vertex.Color = {color.r, color.g, color.b, color.a};
            vertex.UV0 = {texCoord0.x, texCoord0.y};
            vertex.UV1 = {texCoord1.x, texCoord1.y};
            vertex.UV2 = {texCoord2.x, texCoord2.y};
            model->FullVertices.push_back(vertex);
        }
        
        for (size_t f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            model->Indices.push_back(face.mIndices[0]);
            model->Indices.push_back(face.mIndices[2]);
            model->Indices.push_back(face.mIndices[1]);
        }

        auto meshMaterial = scene->mMaterials[mesh->mMaterialIndex];
        BeMaterial material;
        aiString texPath;
        constexpr int diffuseTexIndex = 0;
        if (meshMaterial->GetTexture(aiTextureType_DIFFUSE, diffuseTexIndex, &texPath) == AI_SUCCESS) {
            material.DiffuseTexture = LoadTextureFromAssimpPath(texPath, scene, modelPath.parent_path());
        }
        constexpr int specularTexIndex = 0;
        if (meshMaterial->GetTexture(aiTextureType_SPECULAR, specularTexIndex, &texPath) == AI_SUCCESS) {
            material.SpecularTexture = LoadTextureFromAssimpPath(texPath, scene, modelPath.parent_path());
        }

        aiColor4D color{};
        if (meshMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {;
            material.DiffuseColor = {color.r, color.g, color.b};
        }
        if (meshMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {;
            material.SpecularColor = {color.r, color.g, color.b};
        }
        meshMaterial->Get(AI_MATKEY_SHININESS, material.Shininess);
        
        
        model->DrawSlices.push_back({
            .IndexCount = mesh->mNumFaces * 3,
            .StartIndexLocation = indexOffset,
            .BaseVertexLocation = vertexOffset,
            .Material = material
        });
        
        vertexOffset += mesh->mNumVertices;
        indexOffset += mesh->mNumFaces * 3;
    }
    
    return model;
}

auto BeAssetImporter::LoadTextureFromFile(const std::filesystem::path& texturePath) const -> std::shared_ptr<BeTexture> {
    int w = 0, h = 0, channelsInFile = 0;
    // Force 4 channels to standardize output layout as RGBA8.
    uint8_t* decoded = stbi_load(texturePath.string().c_str(), &w, &h, &channelsInFile, 4);
    if (!decoded) throw std::runtime_error("Failed to load texture from file: " + texturePath.string());

    const size_t imageSize = static_cast<size_t>(w) * static_cast<size_t>(h) * 4; // RGBA8
    const auto pixels = static_cast<uint8_t*>(malloc(imageSize));
    if (!pixels) {
        stbi_image_free(decoded);
        throw std::runtime_error("Failed to allocate texture");
    }
    memcpy(pixels, decoded, imageSize);
    stbi_image_free(decoded);

    auto texture = std::make_shared<BeTexture>();
    texture->Pixels = pixels; // free with free()
    texture->Width = w;
    texture->Height = h;
    texture->FlipVertically();
    texture->CreateSRV(_device);
    return texture;
}

auto BeAssetImporter::LoadTextureFromAssimpPath(
    const aiString& texPath,
    const aiScene* scene,
    const std::filesystem::path& parentPath)
const -> std::shared_ptr<BeTexture> {
    
    if (texPath.C_Str()[0] != '*') {
        return LoadTextureFromFile(parentPath / texPath.C_Str());
    } // use stb_image
            
    char* endPtr;
    const long texIndex = std::strtol(texPath.C_Str() + 1, &endPtr, 10);
    const aiTexture* aiTex = scene->mTextures[texIndex];

    // handle compressed texture
    if (aiTex->mHeight == 0) {
        return LoadTextureFromMemoryEncoded(reinterpret_cast<const uint8_t*>(aiTex->pcData), aiTex->mWidth);
    }

    return LoadTextureFromMemoryDecoded(reinterpret_cast<const uint8_t*>(aiTex->pcData), aiTex->mWidth, aiTex->mHeight);
}

auto BeAssetImporter::LoadTextureFromMemoryEncoded(const uint8_t* data, uint32_t length) const -> std::shared_ptr<BeTexture> {
    int w = 0, h = 0, channelsInFile = 0;

    // Force 4 channels to standardize output layout as RGBA8.
    uint8_t* decoded = stbi_load_from_memory(data, length, &w, &h, &channelsInFile, 4);
    if (!decoded) throw std::runtime_error("Failed to decode texture");

    // Allocate new space for the decoded image using your own allocator (malloc here)
    const size_t imageSize = static_cast<size_t>(w) * static_cast<size_t>(h) * 4; // RGBA8
    const auto pixels = static_cast<uint8_t*>(malloc(imageSize));
    if (!pixels) {
        stbi_image_free(decoded);
        throw std::runtime_error("Failed to allocate texture");
    }
    memcpy(pixels, decoded, imageSize);
    stbi_image_free(decoded);

    auto texture = std::make_shared<BeTexture>();
    texture->Pixels = pixels; // free with free()
    texture->Width = w;
    texture->Height = h;
    texture->FlipVertically();
    texture->CreateSRV(_device);
    return texture;
}

auto BeAssetImporter::LoadTextureFromMemoryDecoded(const uint8_t* data, uint32_t width, uint32_t height) const -> std::shared_ptr<BeTexture> {
    const size_t count = width * height;
    if (count == 0) throw std::runtime_error("Failed to decode texture");

    const auto pixels = static_cast<uint8_t*>(malloc(count * 4));
    if (!pixels) throw std::runtime_error("Failed to allocate texture");

    const uint8_t* src = data;
    for (size_t i = 0; i < count; ++i) {
        pixels[4 * i + 0] = src[4 * i + 2]; //r
        pixels[4 * i + 1] = src[4 * i + 1]; //g
        pixels[4 * i + 2] = src[4 * i + 0]; //b
        pixels[4 * i + 3] = src[4 * i + 3]; //a
    }

    auto texture = std::make_shared<BeTexture>();
    texture->Pixels = pixels; // free with free()
    texture->Width = width;
    texture->Height = height;
    texture->FlipVertically();
    texture->CreateSRV(_device);
    return texture;
}



