#include "BeAssimpImporter.h"

#include <algorithm>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stb_image/stb_image.h>
#include <include-libassert.h>
#include <BeProp.h>
#include <BeTexture.h>

BeAssimpImporter::BeAssimpImporter() {}
BeAssimpImporter::~BeAssimpImporter() {}

auto BeAssimpImporter::LoadProp(const std::filesystem::path& modelPath, const MaterialExtractFunction& materialExtractFunction) -> std::shared_ptr<BeProp> {
    
    constexpr auto flags = (
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_PreTransformVertices |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_CalcTangentSpace |
        aiProcess_ValidateDataStructure |
        aiProcess_OptimizeMeshes
    );
    
    const aiScene* scene = _importer.ReadFile(modelPath.string().c_str(), flags);
    be_assert(scene && scene->mRootNode, "Failed to load model: " + modelPath.string());
    
    auto prop = std::make_shared<BeProp>();
    prop->Mesh = std::make_shared<BeMesh>();

    struct MaterialGroup {
        uint32_t MaterialIndex;
        std::vector<const aiMesh*> Meshes;
    };
    std::vector<MaterialGroup> groups;
    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        auto it = std::ranges::find(groups, mesh->mMaterialIndex, &MaterialGroup::MaterialIndex);
        if (it == groups.end()) {
            groups.push_back({ .MaterialIndex = mesh->mMaterialIndex });
            it = std::prev(groups.end());
        }
        it->Meshes.push_back(mesh);
    }

    size_t numVertices = 0;
    size_t numIndices = 0;
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        numVertices += mesh->mNumVertices;
        numIndices += 3 * mesh->mNumFaces;
    }

    prop->Mesh->Vertices.reserve(numVertices);
    prop->Mesh->Indices.reserve(numIndices);
    prop->Mesh->LocalSlices.reserve(groups.size());
    prop->Surfaces.reserve(groups.size());

    for (const auto& group : groups) {
        const int32_t groupVertexBase = static_cast<int32_t>(prop->Mesh->Vertices.size());
        const uint32_t groupIndexBase = static_cast<uint32_t>(prop->Mesh->Indices.size());

        for (const aiMesh* mesh : group.Meshes) {
            const uint32_t meshVertexOffset = static_cast<uint32_t>(prop->Mesh->Vertices.size()) - static_cast<uint32_t>(groupVertexBase);

            for (size_t v = 0; v < mesh->mNumVertices; ++v) {
                BeFullVertex vertex{};
                aiVector3D position  = mesh->mVertices[v];
                aiVector3D normal    = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.f, 1.f, 0.f);
                aiColor4D  color     = mesh->HasVertexColors(0) ? mesh->mColors[0][v] : aiColor4D(1, 1, 1, 1);
                aiVector3D texCoord0 = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][v] : aiVector3D(0.f, 0.f, 0.f);
                aiVector3D tangent   = mesh->HasTangentsAndBitangents() ? mesh->mTangents[v]   : aiVector3D(1.f, 0.f, 0.f);
                aiVector3D bitangent = mesh->HasTangentsAndBitangents() ? mesh->mBitangents[v] : aiVector3D(0.f, 1.f, 0.f);

                // Negate X on all vectors for the engine's left-handed coordinate system.
                // Compute handedness in engine space so cross(N, T) * w reconstructs the correct bitangent.
                glm::vec3 nNeg = {-normal.x,   normal.y,   normal.z};
                glm::vec3 tNeg = {-tangent.x,  tangent.y,  tangent.z};
                glm::vec3 bNeg = {-bitangent.x, bitangent.y, bitangent.z};
                float handedness = glm::dot(glm::cross(nNeg, tNeg), bNeg) >= 0.0f ? 1.0f : -1.0f;

                vertex.Position = {-position.x, position.y, position.z};
                vertex.Normal      = nNeg;
                vertex.Color    = {color.r, color.g, color.b, color.a};
                vertex.UV0      = {texCoord0.x, texCoord0.y};
                vertex.Tangent  = {tNeg.x, tNeg.y, tNeg.z, handedness};
                prop->Mesh->Vertices.push_back(vertex);
            }

            for (size_t f = 0; f < mesh->mNumFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                if (face.mNumIndices != 3) continue;
                prop->Mesh->Indices.push_back(face.mIndices[0] + meshVertexOffset);
                prop->Mesh->Indices.push_back(face.mIndices[2] + meshVertexOffset);
                prop->Mesh->Indices.push_back(face.mIndices[1] + meshVertexOffset);
            }
        }

        prop->Mesh->LocalSlices.push_back({
            .IndexCount = static_cast<uint32_t>(prop->Mesh->Indices.size()) - groupIndexBase,
            .StartIndexLocation = groupIndexBase,
            .BaseVertexLocation = groupVertexBase,
        });

        auto& surface = prop->Surfaces.emplace_back();
        surface.MeshSlice = static_cast<uint32_t>(prop->Mesh->LocalSlices.size() - 1);
        //TODO:
        // 1. add a nice wrapper around aiMaterial to simplify API
        // 2? add a default load route from SRM
        surface.Material = materialExtractFunction(scene->mMaterials[group.MaterialIndex], scene, modelPath.parent_path(), surface);
    }

    return prop;
}

auto BeAssimpImporter::LoadTextureFromAssimpPath(
    const aiString& texPath,
    const aiScene* scene,
    const std::filesystem::path& parentPath
) -> std::shared_ptr<BeTexture> {
    
    static int tempCount = -1;
    tempCount++;
    auto builder =
        BeTexture::Create("TODO" + std::to_string(tempCount))
        .SetUsage(SenTextureUsage::Sampled)
        .SetFormat(SenFormat::RGBA8_Unorm)
        .SetMipsAuto()
        .GenerateMips();


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
            .Build();
    }

    char* endPtr;
    const long texIndex = std::strtol(texPath.C_Str() + 1, &endPtr, 10);
    const aiTexture* aiTex = scene->mTextures[texIndex];

    // handle compressed texture
    if (aiTex->mHeight == 0) {
        int w = 0, h = 0, channelsInFile = 0;
        uint8_t* decoded = stbi_load_from_memory(reinterpret_cast<const uint8_t*>(aiTex->pcData), aiTex->mWidth, &w, &h, &channelsInFile, 4);
        if (!decoded) throw std::runtime_error("Failed to decode embedded texture");

        BeTexture::FlipVertically(w, h, decoded);
        const auto & resource = builder
            .SetSize(w, h).FillFromMemory(decoded).Build();
        stbi_image_free(decoded);
        return resource;
    }

    // decoded texture
    const size_t pixelCount = aiTex->mWidth * aiTex->mHeight;
    const auto converted = static_cast<uint8_t*>(malloc(pixelCount * 4));
    const uint8_t* srcData = reinterpret_cast<const uint8_t*>(aiTex->pcData);
    for (size_t i = 0; i < pixelCount; ++i) {
        converted[i * 4 + 0] = srcData[i * 4 + 2]; // B -> R
        converted[i * 4 + 1] = srcData[i * 4 + 1]; // G
        converted[i * 4 + 2] = srcData[i * 4 + 0]; // R -> B
        converted[i * 4 + 3] = srcData[i * 4 + 3]; // A
    }
    BeTexture::FlipVertically(aiTex->mWidth, aiTex->mHeight, converted);
    const auto & resource = builder
        .SetSize(aiTex->mWidth, aiTex->mHeight)
        .FillFromMemory(converted)
        .Build()
    ;
    free(converted);
    return resource;
}

