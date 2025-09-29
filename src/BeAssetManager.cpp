#include "BeAssetManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/postprocess.h>

#include "Utils.h"

std::unique_ptr<BeAssetManager> BeAssetManager::Ins = nullptr;


BeAssetManager::BeAssetManager(const ComPtr<ID3D11Device>& device) {
    _device = device;
    
}

auto BeAssetManager::LoadModel(const std::string& name, const std::filesystem::path& modelPath) -> BeModel& {
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

    
    BeModel model;
    model.MeshInstructions.reserve(scene->mNumMeshes);

    size_t numVertices = 0;
    size_t numIndices = 0;
    for (unsigned i=0; i<scene->mNumMeshes; ++i) {
        const auto mesh = scene->mMeshes[i];
        numVertices += mesh->mNumVertices;
        numIndices += 3 * mesh->mNumFaces;
    }

    model.FullVertices.reserve(numVertices); 
    model.Indices.reserve(numIndices);
    
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
            model.FullVertices.push_back(vertex);
        }
        
        for (size_t f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            model.Indices.push_back(face.mIndices[0]);
            model.Indices.push_back(face.mIndices[2]);
            model.Indices.push_back(face.mIndices[1]);
        }

        auto material = scene->mMaterials[mesh->mMaterialIndex];
        BeTexture* texture = nullptr;
        aiString texPath;
        constexpr int diffuseTexIndex = 0;
        if (material->GetTexture(aiTextureType_DIFFUSE, diffuseTexIndex, &texPath) == AI_SUCCESS) {
            texture = &LoadTextureFromAssimpPath(
                name + ".mat" + std::to_string(i) + ".diffuse-tex." + std::to_string(diffuseTexIndex),
                texPath,
                scene,
                modelPath.parent_path()
            );
        }

        
        model.MeshInstructions.push_back({
            .IndexCount = mesh->mNumFaces * 3,
            .StartIndexLocation = indexOffset,
            .BaseVertexLocation = vertexOffset,
            .DiffuseTexture = texture //temporary
        });
        
        vertexOffset += mesh->mNumVertices;
        indexOffset += mesh->mNumFaces * 3;
    }

    _models[name] = model;
    return _models.at(name);
}

auto BeAssetManager::GetModel(const std::string& name) -> BeModel& {
    return _models.at(name);
}

auto BeAssetManager::LoadTextureFromFile(const std::string& name, const std::filesystem::path& texturePath) -> BeTexture&{
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

    BeTexture& texture = _textures[name];
    texture.Pixels = pixels; // free with free()
    texture.Width = w;
    texture.Height = h;
    FlipTextureVertically(texture);
    CreateSRV(texture);
    return texture;
}

auto BeAssetManager::GetTexture(const std::string& name) -> BeTexture& {
    return _textures.at(name);
}

auto BeAssetManager::LoadShader(
    const std::string& name,
    const std::filesystem::path& shaderPath,
    const std::vector<BeVertexElementDescriptor>& vertexLayout)
-> BeShader& {
    
    auto pair = _shaders.try_emplace(name, _device.Get(), shaderPath.wstring(), vertexLayout);
    if (!pair.second) {
        throw std::runtime_error("Shader already exists: " + name);
    }
    return pair.first->second;
}

auto BeAssetManager::LoadTextureFromAssimpPath(
    const std::string& name,
    const aiString& texPath,
    const aiScene* scene,
    const std::filesystem::path& parentPath)
-> BeTexture& {
    
    if (texPath.C_Str()[0] != '*') {
        return LoadTextureFromFile(name, parentPath / texPath.C_Str());
    } // use stb_image
            
    char* endPtr;
    const long texIndex = std::strtol(texPath.C_Str() + 1, &endPtr, 10);
    const aiTexture* aiTex = scene->mTextures[texIndex];

    // handle compressed texture
    if (aiTex->mHeight == 0) {
        return LoadTextureFromMemoryEncoded(name, reinterpret_cast<const uint8_t*>(aiTex->pcData), aiTex->mWidth);
    }

    return LoadTextureFromMemoryDecoded(name, reinterpret_cast<const uint8_t*>(aiTex->pcData), aiTex->mWidth, aiTex->mHeight);
}

auto BeAssetManager::LoadTextureFromMemoryEncoded(const std::string& name, const uint8_t* data, const uint32_t length) -> BeTexture& {

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

    BeTexture& texture = _textures[name];
    texture.Pixels = pixels; // free with free()
    texture.Width = w;
    texture.Height = h;
    FlipTextureVertically(texture);
    CreateSRV(texture);
    return texture;
}

auto BeAssetManager::LoadTextureFromMemoryDecoded(const std::string& name, const uint8_t* data, const uint32_t width, const uint32_t height) -> BeTexture& {
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

    BeTexture& texture = _textures[name];
    texture.Pixels = pixels;    // free with free()
    texture.Width = width;
    texture.Height = height;
    FlipTextureVertically(texture);
    CreateSRV(texture);
    return texture;
}


// ReSharper disable once CppMemberFunctionMayBeStatic
// ReSharper disable once CppParameterMayBeConstPtrOrRef
auto BeAssetManager::FlipTextureVertically(BeTexture& texture) -> void {
    const uint32_t rowSize = texture.Width * 4; // 4 bytes per pixel (RGBA8)
    const auto tempRow = new uint8_t[rowSize];

    for (uint32_t y = 0; y < texture.Height / 2; ++y) {
        uint8_t* topRow = texture.Pixels + y * rowSize;
        uint8_t* bottomRow = texture.Pixels + (texture.Height - 1 - y) * rowSize;

        // Swap topRow and bottomRow
        memcpy(tempRow, topRow, rowSize);
        memcpy(topRow, bottomRow, rowSize);
        memcpy(bottomRow, tempRow, rowSize);
    }

    delete[] tempRow;
}

auto BeAssetManager::CreateSRV(BeTexture& texture) -> void {
    const D3D11_TEXTURE2D_DESC desc = {
        .Width = texture.Width,
        .Height = texture.Height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = { .Count = 1 },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = 0,
        .MiscFlags = 0, 
    };

    const D3D11_SUBRESOURCE_DATA initData = {
        .pSysMem = texture.Pixels,
        .SysMemPitch = 4 * texture.Width
    };
            
    ComPtr<ID3D11Texture2D> d3dTexture = nullptr;
    Utils::Check << _device->CreateTexture2D(&desc, &initData, &d3dTexture);
            
    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDescriptor = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
        .Texture2D = { .MostDetailedMip = 0, .MipLevels = 1 },
    };
    Utils::Check << _device->CreateShaderResourceView(d3dTexture.Get(), &srvDescriptor, texture.SRV.GetAddressOf());
}
