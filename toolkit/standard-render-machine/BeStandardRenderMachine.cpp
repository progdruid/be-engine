#include "BeStandardRenderMachine.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeMaterial.h"
#include "assimp-import/BeAssimpImporter.h"
#include <assimp/material.h>
#include "standard-render-machine/BeStandardShadowPass.h"
#include "standard-render-machine/BeStandardGeometryPass.h"
#include "standard-render-machine/BeStandardLightingPass.h"
#include "standard-render-machine/BeStandardBloomPass.h"
#include "standard-render-machine/BeStandardFullscreenEffectPass.h"
#include "standard-render-machine/BeStandardBackbufferPass.h"
#include "standard-render-machine/BeStandardEnvironmentBakePass.h"
#include "standard-render-machine/BeStandardSkyboxPass.h"

auto BeSRMGeometryEntry::CalculateModelMatrix(glm::vec3 pos, glm::quat rot, glm::vec3 scale) -> glm::mat4 {
    return
        glm::translate(glm::mat4(1.0f), pos) *
        glm::mat4_cast(rot) *
        glm::scale(glm::mat4(1.0f), scale);
}

auto BeSRMSunLightEntry::CalculateViewProj(
    glm::vec3 direction,
    float shadowCameraDistance,
    float shadowMapWorldSize,
    float shadowNearPlane,
    float shadowFarPlane
) -> glm::mat4 {
    const float halfSize = shadowMapWorldSize * 0.5f;
    const glm::mat4 lightOrtho = glm::orthoLH_ZO(-halfSize, halfSize, -halfSize, halfSize, shadowNearPlane, shadowFarPlane);
    const glm::vec3 lightPos = -direction * shadowCameraDistance;
    const glm::mat4 lightView = glm::lookAtLH(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    return lightOrtho * lightView;
}

BeStandardRenderMachine::BeStandardRenderMachine(std::weak_ptr<BeRenderer> renderer, BeAssetRegistry& assetRegistry, uint32_t width, uint32_t height)
    : _renderer(std::move(renderer)), _assetRegistry(assetRegistry), _width(width), _height(height) {}

BeStandardRenderMachine::~BeStandardRenderMachine() = default;


// =====================================================================================================================
// BeStandardRenderMachine — texture registry
// =====================================================================================================================


auto BeStandardRenderMachine::DeclareGBufferTarget(const std::string& name, SenFormat format) -> std::shared_ptr<BeTexture> {
    auto texture = DeclareTexture(name, format, false, 1);
    _textureRegistry[name] = texture;
    _gbufferTargets.push_back(texture);
    return texture;
}

auto BeStandardRenderMachine::DeclareDepth(const std::string& name, SenFormat format) -> std::shared_ptr<BeTexture> {
    auto texture = BeTexture::Create(name)
        .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
        .SetFormat(format)
        .SetSize(_width, _height)
        .Build();

    _textureRegistry[name] = texture;
    _depthTarget = texture;
    return texture;
}

auto BeStandardRenderMachine::DeclareTexture(const std::string& name, SenFormat format, bool storage, uint32_t mips) -> std::shared_ptr<BeTexture> {
    auto usage = SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource;
    if (storage) {
        usage = usage | SenTextureUsage::Storage;
    }

    auto texture = BeTexture::Create(name)
        .SetUsage(usage)
        .SetFormat(format)
        .SetSize(_width, _height)
        .SetMips(mips)
        .Build();

    _textureRegistry[name] = texture;
    return texture;
}

auto BeStandardRenderMachine::GetRenderTexture(const std::string& name) const -> std::shared_ptr<BeTexture> {
    auto it = _textureRegistry.find(name);
    return it != _textureRegistry.end() ? it->second : nullptr;
}

// =====================================================================================================================
// BeStandardRenderMachine — pass builders
// =====================================================================================================================

auto BeStandardRenderMachine::AddShadowPass() -> void {
    auto pass = std::make_unique<BeStandardShadowPass>(this);
    _passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddGeometryPass() -> void {
    be_assert(!_gbufferTargets.empty(), "No G-buffer targets declared before AddGeometryPass");
    be_assert(_depthTarget, "No depth target declared before AddGeometryPass");

    auto pass = std::make_unique<BeStandardGeometryPass>(this, _gbufferTargets, _depthTarget);
    _passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddLightingPass(const std::string& outputName) -> void {
    be_assert(!_gbufferTargets.empty(), "No G-buffer targets declared before AddLightingPass");
    be_assert(_depthTarget, "No depth target declared before AddLightingPass");

    auto output = GetRenderTexture(outputName);
    be_assert(output, "AddLightingPass: output texture not found: " + outputName);

    auto pass = std::make_unique<BeStandardLightingPass>(this, _gbufferTargets, _depthTarget, output);
    _passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddBloomPass(
    uint32_t mipCount,
    const std::string& inputName,
    const std::string& outputName,
    std::shared_ptr<BeTexture> dirtTexture
) -> void {
    auto input  = GetRenderTexture(inputName);
    auto output = GetRenderTexture(outputName);
    be_assert(input,  "AddBloomPass: input texture not found: "  + inputName);
    be_assert(output, "AddBloomPass: output texture not found: " + outputName);

    auto bloomTexture = DeclareTexture("__standard_bloom", input->Format, false, mipCount);

    if (!dirtTexture) {
        dirtTexture = BeAssetRegistry::GetDefaultTexture("black").lock();
    }

    auto pass = std::make_unique<BeStandardBloomPass>(this, input, bloomTexture, output, dirtTexture, mipCount);
    _passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddFullscreenPass(
    std::weak_ptr<BeShader> shader,
    std::shared_ptr<BeMaterial> material,
    const std::vector<std::string>& outputNames
) -> void {
    std::vector<std::shared_ptr<BeTexture>> outputs;
    outputs.reserve(outputNames.size());
    for (const auto& name : outputNames) {
        auto tex = GetRenderTexture(name);
        be_assert(tex, "AddFullscreenPass: output texture not found: " + name);
        outputs.push_back(tex);
    }

    auto pass = std::make_unique<BeStandardFullscreenEffectPass>(this, shader, material, std::move(outputs));
    _passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddBackbufferPass(const std::string& inputName, glm::vec3 clearColor) -> void {
    auto input = GetRenderTexture(inputName);
    be_assert(input, "AddBackbufferPass: input texture not found: " + inputName);

    auto pass = std::make_unique<BeStandardBackbufferPass>(this, input, clearColor);
    _passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddPass(std::unique_ptr<BeRenderPass> pass) -> void {
    _passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddEnvironmentBakePass(std::shared_ptr<BeTexture> equirect, uint32_t cubemapSize) -> void {
    _envCubemap = BeTexture::Create("__standard_env_cubemap")
        .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA16_Float)
        .SetCubemap(true)
        .SetSize(cubemapSize, cubemapSize)
        .Build();

    _environmentBakePass = std::make_unique<BeStandardEnvironmentBakePass>(this, std::move(equirect), _envCubemap);
}

auto BeStandardRenderMachine::BakeEnvironment() -> void {
    be_assert(_environmentBakePass, "BakeEnvironment: AddEnvironmentBakePass was not called");
    _environmentBakePass->Initialise();
    _renderer.lock()->RenderOnce({ _environmentBakePass.get() });
}

auto BeStandardRenderMachine::AddSkyboxPass(const std::string& outputName, float clampRadiance) -> void {
    be_assert(_envCubemap, "AddSkyboxPass: AddEnvironmentBakePass must be called first");
    be_assert(_depthTarget, "AddSkyboxPass: no depth target declared");

    auto output = GetRenderTexture(outputName);
    be_assert(output, "AddSkyboxPass: output texture not found: " + outputName);

    _passes.push_back(std::make_unique<BeStandardSkyboxPass>(this, _depthTarget, _envCubemap, output, clampRadiance));
}

// =====================================================================================================================
// BeStandardRenderMachine — build
// =====================================================================================================================

auto BeStandardRenderMachine::ClearPasses() -> void {
    _renderer.lock()->ClearPasses();
    _passes.clear();
}

auto BeStandardRenderMachine::BuildPasses() -> void {
    auto renderer = _renderer.lock();
    for (auto& pass : _passes)
        renderer->AddRenderPass(pass.get());
    for (auto& pass : _passes)
        pass->Initialise();
}


// =====================================================================================================================
// BeStandardRenderMachine — debug
// =====================================================================================================================


auto BeStandardRenderMachine::SetDebugChannel(int channel) -> void {
    _debugChannel = channel;
}

auto BeStandardRenderMachine::GetDebugChannelTexture() const -> std::shared_ptr<BeTexture> {
    if (_debugChannel < 0 || _debugChannel >= static_cast<int>(_gbufferTargets.size()))
        return nullptr;
    return _gbufferTargets[_debugChannel];
}

// =====================================================================================================================
// BeStandardRenderMachine — frame submission
// =====================================================================================================================

auto BeStandardRenderMachine::ClearFrame() -> void {
    _objectMaterialCursor = 0;
    _geometryEntries.clear();
    _sunLightEntries.clear();
    _pointLightEntries.clear();
}

auto BeStandardRenderMachine::AddGeometry(const BeSRMGeometryEntry& entry) -> void {
    _geometryEntries.push_back(entry);
}

auto BeStandardRenderMachine::AddSunLight(const BeSRMSunLightEntry& entry) -> void {
    _sunLightEntries.push_back(entry);
}

auto BeStandardRenderMachine::AddPointLight(const BeSRMPointLightEntry& entry) -> void {
    _pointLightEntries.push_back(entry);
}

auto BeStandardRenderMachine::GetGeometryEntries() const -> const std::vector<BeSRMGeometryEntry>& {
    return _geometryEntries;
}

auto BeStandardRenderMachine::GetSunLightEntries() const -> const std::vector<BeSRMSunLightEntry>& {
    return _sunLightEntries;
}

auto BeStandardRenderMachine::GetPointLightEntries() const -> const std::vector<BeSRMPointLightEntry>& {
    return _pointLightEntries;
}

// =====================================================================================================================
// BeStandardRenderMachine — asset loading
// =====================================================================================================================

auto BeStandardRenderMachine::LoadProp(
    const std::filesystem::path& modelPath,
    std::weak_ptr<BeShader> shader,
    BeSRMLightingModel model
) -> std::shared_ptr<BeProp> {

    std::function<std::shared_ptr<BeMaterial>(aiMaterial const*, aiScene const*, const std::filesystem::path&)> materialExtractFunction;

    if (model == BeSRMLightingModel::PBR) {
        materialExtractFunction = [shader](aiMaterial const* mat, aiScene const* scene, const std::filesystem::path& parentPath) -> std::shared_ptr<BeMaterial> {
            const auto scheme = shader.lock()->GetMaterialScheme("geometry-main");
            auto material = BeMaterial::Create(scheme, true);

            aiString texPath;
            if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
                mat->GetTexture(aiTextureType_DIFFUSE,    0, &texPath) == AI_SUCCESS) {
                material->SetTexture("Diffuse_or_Albedo", BeAssimpImporter::LoadTextureFromAssimpPath(texPath, scene, parentPath));
            }
            if (mat->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS) {
                material->SetTexture("ORM_RGB", BeAssimpImporter::LoadTextureFromAssimpPath(texPath, scene, parentPath));
            }
            if (mat->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
                material->SetTexture("Emissive_RGB", BeAssimpImporter::LoadTextureFromAssimpPath(texPath, scene, parentPath));
            }
            if (mat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
                material->SetTexture("NormalMap", BeAssimpImporter::LoadTextureFromAssimpPath(texPath, scene, parentPath));
            }

            aiColor4D color{};
            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
                material->SetFloat3("BaseColor", {color.r, color.g, color.b});

            return material;
        };
    } else {
        materialExtractFunction = [shader](aiMaterial const* mat, aiScene const* scene, const std::filesystem::path& parentPath) -> std::shared_ptr<BeMaterial> {
            const auto scheme = shader.lock()->GetMaterialScheme("geometry-main");
            auto material = BeMaterial::Create(scheme, true);

            aiString texPath;
            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                material->SetTexture("DiffuseTexture", BeAssimpImporter::LoadTextureFromAssimpPath(texPath, scene, parentPath));
            }
            if (mat->GetTexture(aiTextureType_SPECULAR, 0, &texPath) == AI_SUCCESS) {
                material->SetTexture("SpecularTexture", BeAssimpImporter::LoadTextureFromAssimpPath(texPath, scene, parentPath));
            }
            if (mat->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
                material->SetTexture("EmissiveTexture", BeAssimpImporter::LoadTextureFromAssimpPath(texPath, scene, parentPath));
            }
            if (mat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
                material->SetTexture("NormalMap", BeAssimpImporter::LoadTextureFromAssimpPath(texPath, scene, parentPath));
            }

            aiColor4D color{};
            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
                material->SetFloat3("DiffuseColor", {color.r, color.g, color.b});

            aiColor4D specColor{};
            if (mat->Get(AI_MATKEY_COLOR_SPECULAR, specColor) == AI_SUCCESS)
                material->SetFloat3("SpecularColor", {specColor.r, specColor.g, specColor.b});

            float shininess = 0.0f;
            if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
                material->SetFloat1("Shininess", shininess / 2048.0f);

            return material;
        };
    }

    static BeAssimpImporter importer;
    auto prop = importer.LoadProp(modelPath, shader, materialExtractFunction);
    RegisterMesh(prop->Mesh);
    return prop;
}


// =====================================================================================================================
// BeStandardRenderMachine — mesh baking
// =====================================================================================================================

auto BeStandardRenderMachine::RegisterMesh(const std::shared_ptr<BeMesh>& mesh) -> void {
    _registeredMeshes.push_back(mesh);
}

auto BeStandardRenderMachine::BakeMeshes() -> void {
    size_t totalVertices = 0;
    size_t totalIndices  = 0;
    for (const auto& mesh : _registeredMeshes) {
        totalVertices += mesh->Vertices.size();
        totalIndices  += mesh->Indices.size();
    }

    std::vector<BeFullVertex> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(totalVertices);
    indices.reserve(totalIndices);

    for (const auto& mesh : _registeredMeshes) {
        if (_meshSlices.contains(mesh.get()))
            continue;

        const auto vertexBase = static_cast<int32_t>(vertices.size());
        const auto indexBase  = static_cast<uint32_t>(indices.size());

        vertices.insert(vertices.end(), mesh->Vertices.begin(), mesh->Vertices.end());
        indices.insert(indices.end(),   mesh->Indices.begin(),  mesh->Indices.end());

        auto& slices = _meshSlices[mesh.get()];
        for (const auto& slice : mesh->Slices) {
            slices.push_back({
                .IndexCount          = slice.IndexCount,
                .StartIndexLocation  = slice.StartIndexLocation + indexBase,
                .BaseVertexLocation  = slice.BaseVertexLocation + vertexBase,
            });
        }
    }

    _sharedVertexBuffer = SenBackend::CreateBuffer({
        .Usage  = SenBufferUsage::Vertex,
        .Access = SenBufferAccess::Immutable,
        .Size   = static_cast<uint32_t>(vertices.size() * sizeof(BeFullVertex)),
        .Data   = vertices.data(),
    });

    _sharedIndexBuffer = SenBackend::CreateBuffer({
        .Usage  = SenBufferUsage::Index,
        .Access = SenBufferAccess::Immutable,
        .Size   = static_cast<uint32_t>(indices.size() * sizeof(uint32_t)),
        .Data   = indices.data(),
    });
}

// =====================================================================================================================
// BeStandardRenderMachine — internal
// =====================================================================================================================

/* TODO: this is wrong and needs to be fixed. 
 * 
 * basically geometry passes like `BeStandardGeometryPass` or `BeStandardShadowPass`
 * all need a dedicated object material per each object they render.
 * therefore, this pooling was added to mitigate perf impact at least a bit.
 * 
 * practically, this has to be solved on `sen-rhi` level.
 * possible solutions that i see: 
 *      ring buffer option for `SenBuffer` with bump on every write;
 *      structured buffer option for `SenBuffer` with enough slots for every object
 */
auto BeStandardRenderMachine::AcquireNewObjectMaterial(const BeMaterialScheme& scheme) -> std::shared_ptr<BeMaterial> {
    if (_objectMaterialCursor >= _objectMaterialPool.size()) {
        _objectMaterialPool.push_back(BeMaterial::Create(scheme, true));
    } else if (_objectMaterialPool[_objectMaterialCursor]->GetSchemeName() != scheme.Name) {
        _objectMaterialPool[_objectMaterialCursor] = BeMaterial::Create(scheme, true);
    }
    return _objectMaterialPool[_objectMaterialCursor++];
}
