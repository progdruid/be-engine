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

    std::vector<glm::vec3> positions; positions.reserve(numVertices);
    std::vector<glm::vec3> normals;   normals.reserve(numVertices);
    std::vector<glm::vec4> colors;    colors.reserve(numVertices);
    std::vector<glm::vec2> uv0s;      uv0s.reserve(numVertices);
    std::vector<glm::vec2> uv1s;      uv1s.reserve(numVertices);
    std::vector<glm::vec2> uv2s;      uv2s.reserve(numVertices);
    std::vector<uint32_t> indices;    indices.reserve(numIndices);  

    int32_t vertexOffset = 0; // indexing in each mesh starts from 0
    uint32_t indexOffset = 0;
    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        
        for (size_t v = 0; v < mesh->mNumVertices; ++v) {
            aiVector3D position = mesh->mVertices[v];
            aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.f, 1.f, 0.f);

            //temporary
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiColor4D color;
            aiColor3D matColor;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, matColor) == AI_SUCCESS) {
                color = aiColor4D(matColor.r, matColor.g, matColor.b, 1.f);
            }

            //aiColor4D color = mesh->HasVertexColors(0) ? mesh->mColors[v][0] : aiColor4D(1.f, 1.f, 1.f, 1.f);
            aiVector3D texCoord0 = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][v] : aiVector3D(0.f, 0.f, 0.f);
            aiVector3D texCoord1 = mesh->HasTextureCoords(1) ? mesh->mTextureCoords[1][v] : aiVector3D(0.f, 0.f, 0.f);
            aiVector3D texCoord2 = mesh->HasTextureCoords(2) ? mesh->mTextureCoords[2][v] : aiVector3D(0.f, 0.f, 0.f);
            positions.emplace_back(-position.x, position.y, position.z);
            normals.emplace_back(-normal.x, normal.y, normal.z);
            colors.emplace_back(color.r, color.g, color.b, color.a);
            uv0s.emplace_back(texCoord0.x, texCoord0.y);
            uv1s.emplace_back(texCoord1.x, texCoord1.y);
            uv2s.emplace_back(texCoord2.x, texCoord2.y);
        }
        
        for (size_t f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[2]);
            indices.push_back(face.mIndices[1]);
        }

        DrawInstructions.push_back({
            .IndexCount = mesh->mNumFaces * 3,
            .StartIndexLocation = indexOffset,
            .BaseVertexLocation = vertexOffset});
        
        vertexOffset += mesh->mNumVertices;
        indexOffset += mesh->mNumFaces * 3;
    }

    
    //here we create buffers
    uint32_t vertexSize = 0;
    for (const auto& element : Shader->VertexLayout) {
        vertexSize += BeVertexElementDescriptor::SemanticSizes.at(element.Attribute);
    }
    
    D3D11_BUFFER_DESC vertexBufferDescriptor = {};
    vertexBufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDescriptor.ByteWidth = static_cast<UINT>(numVertices * vertexSize);
        
    std::vector<uint8_t> vertexBufferData(vertexBufferDescriptor.ByteWidth);
    uint8_t* ptr = vertexBufferData.data();
    for (size_t i = 0; i < numVertices; ++i) {
    for (const auto& [Name, Attribute] : Shader->VertexLayout) {
        switch (Attribute) {
        case BeVertexElementDescriptor::BeVertexSemantic::Position: {
            memcpy(ptr, &positions[i], sizeof(glm::vec3));
            ptr += sizeof(glm::vec3);
            break;
        }
        case BeVertexElementDescriptor::BeVertexSemantic::Normal: {
            memcpy(ptr, &normals[i], sizeof(glm::vec3));
            ptr += sizeof(glm::vec3);
            break;
        }
        case BeVertexElementDescriptor::BeVertexSemantic::Color3: {
            memcpy(ptr, &colors[i], sizeof(glm::vec3));
            ptr += sizeof(glm::vec3);
            break;
        }
        case BeVertexElementDescriptor::BeVertexSemantic::Color4: {
            memcpy(ptr, &colors[i], sizeof(glm::vec4));
            ptr += sizeof(glm::vec4);
            break;
        }
        case BeVertexElementDescriptor::BeVertexSemantic::TexCoord0: {
            memcpy(ptr, &uv0s[i], sizeof(glm::vec2));
            ptr += sizeof(glm::vec2);
            break;
        }
        case BeVertexElementDescriptor::BeVertexSemantic::TexCoord1: {
            memcpy(ptr, &uv1s[i], sizeof(glm::vec2));
            ptr += sizeof(glm::vec2);
            break;
        }
        case BeVertexElementDescriptor::BeVertexSemantic::TexCoord2: {
            memcpy(ptr, &uv2s[i], sizeof(glm::vec2));
            ptr += sizeof(glm::vec2);
            break;
        }
        case BeVertexElementDescriptor::BeVertexSemantic::_Count:
        default:
            throw std::runtime_error("Unsupported vertex attribute in shader layout");
        }
    }}
        
    D3D11_SUBRESOURCE_DATA vertexData;
    vertexData.pSysMem = vertexBufferData.data();
    Utils::ThrowIfFailed(device->CreateBuffer(&vertexBufferDescriptor, &vertexData, VertexBuffer.GetAddressOf()));
    Stride = vertexSize;
    Offset = 0;
    
    D3D11_BUFFER_DESC indexBufferDescriptor = {};
    indexBufferDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDescriptor.ByteWidth = static_cast<UINT>(numIndices * sizeof(uint32_t));
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();
    Utils::ThrowIfFailed(device->CreateBuffer(&indexBufferDescriptor, &indexData, IndexBuffer.GetAddressOf()));
}
