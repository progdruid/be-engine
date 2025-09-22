#include "BeModel.h"

#include <glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Utils.h"

BeModel::BeModel(
    const std::shared_ptr<BeShader>& shader,
    const std::filesystem::path& modelPath,
    ID3D11Device* device) {
    Shader = shader;

    static Assimp::Importer importer;
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


    
    DrawInstructions.reserve(scene->mNumMeshes);
    
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

        DrawInstructions.push_back({
            .IndexCount = mesh->mNumFaces * 3,
            .StartIndexLocation = indexOffset,
            .BaseVertexLocation = vertexOffset});
        
        vertexOffset += mesh->mNumVertices;
        indexOffset += mesh->mNumFaces * 3;
    }

    return;
    
    // //here we create buffers
    // uint32_t vertexSize = 0;
    // for (const auto& element : Shader->VertexLayout) {
    //     vertexSize += BeVertexElementDescriptor::SemanticSizes.at(element.Attribute);
    // }
    //
    // D3D11_BUFFER_DESC vertexBufferDescriptor = {};
    // vertexBufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    // vertexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    // vertexBufferDescriptor.ByteWidth = static_cast<UINT>(numVertices * vertexSize);
    //     
    //
    //     
    // D3D11_SUBRESOURCE_DATA vertexData;
    // vertexData.pSysMem = vertexBufferData.data();
    // Utils::ThrowIfFailed(device->CreateBuffer(&vertexBufferDescriptor, &vertexData, VertexBuffer.GetAddressOf()));
    // Stride = vertexSize;
    // Offset = 0;
    //
    // D3D11_BUFFER_DESC indexBufferDescriptor = {};
    // indexBufferDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    // indexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    // indexBufferDescriptor.ByteWidth = static_cast<UINT>(numIndices * sizeof(uint32_t));
    // D3D11_SUBRESOURCE_DATA indexData = {};
    // indexData.pSysMem = indices.data();
    // Utils::ThrowIfFailed(device->CreateBuffer(&indexBufferDescriptor, &indexData, IndexBuffer.GetAddressOf()));
}
