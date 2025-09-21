#include "ModelAsset.h" 

#include <glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


ModelAsset::ModelAsset(const std::filesystem::path& filePath) {
    // Create buffers for objects
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

    const aiScene* scene = importer.ReadFile(filePath.string().c_str(), flags);
    if (!scene || !scene->mRootNode)
        throw std::runtime_error("Failed to load model: " + filePath.string()); 


    size_t numVertices = 0;
    size_t numIndices = 0;
    for (unsigned i=0; i<scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        numVertices += mesh->mNumVertices;
        numIndices += 3 * mesh->mNumFaces;
    }

    Vertices.reserve(numVertices);
    Colors.reserve(numVertices);
    Indices.reserve(numIndices);
    DrawInstructions.reserve(scene->mNumMeshes);

    int32_t vertexOffset = 0; // indexing in each mesh starts from 0
    uint32_t indexOffset = 0;
    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];

        for (size_t v = 0; v < mesh->mNumVertices; ++v) {
            aiVector3D p = mesh->mVertices[v];
            Vertices.emplace_back(-p.x, p.y, p.z);
            Colors.emplace_back(1, (float)i / scene->mNumMeshes, 0);
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
    
    
}
