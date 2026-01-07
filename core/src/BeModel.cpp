#include "BeModel.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "stb_image/stb_image.h"

#include "BeShader.h"
#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "Utils.h"

auto BeModel::Create(
    const std::filesystem::path& modelPath,
    std::weak_ptr<BeShader> usedShaderForMaterials,
    const BeRenderer& renderer
) -> std::shared_ptr<BeModel> {
    constexpr auto flags = (
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_CalcTangentSpace |
        aiProcess_ValidateDataStructure |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelPath.string().c_str(), flags);
    if (!scene || !scene->mRootNode)
        throw std::runtime_error("Failed to load model: " + modelPath.string());
    
    auto model = std::make_shared<BeModel>();
    model->Shader = usedShaderForMaterials.lock();
    model->DrawSlices.reserve(scene->mNumMeshes);
    const auto& materialScheme = BeAssetRegistry::GetMaterialScheme(model->Shader->GetMaterialSchemeName("geometry-main"));

    std::unordered_map<uint32_t, std::shared_ptr<BeMaterial>> assimpIndexToMaterial;
    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        const auto assimpMaterialIndex = mesh->mMaterialIndex;

        const auto it = assimpIndexToMaterial.find(assimpMaterialIndex);
        if (it != assimpIndexToMaterial.end())
            continue;

        auto material = BeMaterial::Create("mat" + std::to_string(assimpMaterialIndex), materialScheme, true, renderer);
        assimpIndexToMaterial[assimpMaterialIndex] = material;

        const auto meshMaterial = scene->mMaterials[assimpMaterialIndex];
        
        aiString texPath;
        constexpr int diffuseTexIndex = 0;
        if (meshMaterial->GetTexture(aiTextureType_DIFFUSE, diffuseTexIndex, &texPath) == AI_SUCCESS) {
            auto texture = LoadTextureFromAssimpPath(texPath, scene, modelPath.parent_path(), renderer);
            material->SetTexture("DiffuseTexture", texture);
        }
        constexpr int specularTexIndex = 0;
        if (meshMaterial->GetTexture(aiTextureType_SPECULAR, specularTexIndex, &texPath) == AI_SUCCESS) {
            auto texture = LoadTextureFromAssimpPath(texPath, scene, modelPath.parent_path(), renderer);
            material->SetTexture("SpecularTexture", texture);
        }

        aiColor4D color{};
        if (meshMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            material->SetFloat3("DiffuseColor", {color.r, color.g, color.b});
        }
        if (meshMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            material->SetFloat3("SpecularColor", {color.r, color.g, color.b});
        }
        float shininess = 0.f;
        if (meshMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            material->SetFloat("Shininess", shininess);
        }

        material->SetSampler("InputSampler", BeAssetRegistry::GetSampler("point-clamp"));
    }

    for (const auto & material : assimpIndexToMaterial | std::views::values) {
        model->Materials.push_back(material);
    } 
    
    size_t numVertices = 0;
    size_t numIndices = 0;
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        numVertices += mesh->mNumVertices;
        numIndices += 3 * mesh->mNumFaces;
    }

    model->FullVertices.reserve(numVertices);
    model->Indices.reserve(numIndices);
    
    int32_t vertexOffset = 0;
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

        model->DrawSlices.push_back({
            .IndexCount = mesh->mNumFaces * 3,
            .StartIndexLocation = indexOffset,
            .BaseVertexLocation = vertexOffset,
            .Material = assimpIndexToMaterial.at(mesh->mMaterialIndex),
        });

        vertexOffset += mesh->mNumVertices;
        indexOffset += mesh->mNumFaces * 3;
    }

    return model;
}

auto BeModel::LoadTextureFromAssimpPath(
    const aiString& texPath,
    const aiScene* scene,
    const std::filesystem::path& parentPath,
    const BeRenderer& renderer
)
    -> std::shared_ptr<BeTexture> {
    const auto device = renderer.GetDevice();

    static int tempCount = -1;
    tempCount++;
    auto builder =
        BeTexture::Create("TODO" + std::to_string(tempCount))
        .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
        .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
    

    if (texPath.C_Str()[0] != '*') {
        const auto filename = std::filesystem::path(texPath.C_Str()).filename();
        std::filesystem::path path;
        if (!std::filesystem::exists(path = parentPath / filename) &&
            !std::filesystem::exists(path = parentPath / "textures" / filename) &&
            !std::filesystem::exists(path = parentPath / "images" / filename)) {
            throw std::runtime_error("Texture file not found: " + filename.string());
        }
        return builder
            .LoadFromFile(path)
            .AddToRegistry()
            .Build(device);
    }

    char* endPtr;
    const long texIndex = std::strtol(texPath.C_Str() + 1, &endPtr, 10);
    const aiTexture* aiTex = scene->mTextures[texIndex];

    // handle compressed texture
    if (aiTex->mHeight == 0) {
        int w = 0, h = 0, channelsInFile = 0;
        uint8_t* decoded = stbi_load_from_memory(reinterpret_cast<const uint8_t*>(aiTex->pcData), aiTex->mWidth, &w, &h, &channelsInFile, 4);
        if (!decoded) throw std::runtime_error("Failed to decode embedded texture");

        const auto & resource = builder
            .SetSize(w, h).FillFromMemory(decoded).AddToRegistry().Build(device);
        stbi_image_free(decoded);
        return resource;
    }

    // decoded texture
    const size_t pixelCount = aiTex->mWidth * aiTex->mHeight;
    uint8_t* converted = static_cast<uint8_t*>(malloc(pixelCount * 4));
    const uint8_t* srcData = reinterpret_cast<const uint8_t*>(aiTex->pcData);
    for (size_t i = 0; i < pixelCount; ++i) {
        converted[i * 4 + 0] = srcData[i * 4 + 2]; // B -> R
        converted[i * 4 + 1] = srcData[i * 4 + 1]; // G
        converted[i * 4 + 2] = srcData[i * 4 + 0]; // R -> B
        converted[i * 4 + 3] = srcData[i * 4 + 3]; // A
    }
    const auto & resource = builder
        .SetSize(aiTex->mWidth, aiTex->mHeight).FillFromMemory(converted).AddToRegistry().Build(device);
    free(converted);
    return resource;
}
