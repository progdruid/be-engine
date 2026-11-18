#include "BeStandardRenderMachine.h"

#include <include-libassert.h>
#include <Sen.h>

#include "BeAssetRegistry.h"
#include "BeBackend.h"
#include "BeShaderLibrary.h"
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

BeStandardRenderMachine::BeStandardRenderMachine(std::weak_ptr<BeRenderer> renderer, uint32_t width, uint32_t height)
    : _renderer(std::move(renderer)), _width(width), _height(height) {
    UniformMaterial = BeMaterial::Create(BeShaderLibrary::GetMaterialScheme("uniform-material"));
}

BeStandardRenderMachine::~BeStandardRenderMachine() = default;


// =====================================================================================================================
// BeStandardRenderMachine -texture registry
// =====================================================================================================================

auto BeStandardRenderMachine::ClearTargets() -> void {
    _textureRegistry.clear();
    _gbufferTargets.clear();
    _depthTarget = nullptr;
}

auto BeStandardRenderMachine::DeclareGBufferTarget(const std::string& name, SenFormat format) -> std::shared_ptr<BeTexture> {
    auto texture = DeclareTextureTarget(name, format, false, 1);
    _textureRegistry[name] = texture;
    _gbufferTargets.push_back(texture);
    return texture;
}

auto BeStandardRenderMachine::DeclareDepthTarget(const std::string& name, SenFormat format) -> std::shared_ptr<BeTexture> {
    auto texture = BeTexture::Create(name)
        .SetUsage(SenTextureUsage::DepthTarget | SenTextureUsage::Sampled)
        .SetFormat(format)
        .SetSize(_width, _height)
        .Build();

    _textureRegistry[name] = texture;
    _depthTarget = texture;
    return texture;
}

auto BeStandardRenderMachine::DeclareTextureTarget(const std::string& name, SenFormat format, bool storage, uint32_t mips) -> std::shared_ptr<BeTexture> {
    auto usage = SenTextureUsage::ColorTarget | SenTextureUsage::Sampled;
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

auto BeStandardRenderMachine::PollRenderer() -> void {
    const auto renderer = _renderer.lock();
    be_assert(renderer, "BeStandardRenderMachine::PollRenderer: renderer expired");

    const uint32_t width = renderer->GetSwapchainPixelWidth();
    const uint32_t height = renderer->GetSwapchainPixelHeight();
    if (width == 0 || height == 0) { return; }
    if (width == _width && height == _height) { return; }

    Resize(width, height);
}

auto BeStandardRenderMachine::Resize(uint32_t width, uint32_t height) -> void {
    _width = width;
    _height = height;
    for (const auto& texture : _textureRegistry | std::views::values) {
        texture->Resize(width, height);
    }
}

// =====================================================================================================================
// BeStandardRenderMachine -pass builders
// =====================================================================================================================

auto BeStandardRenderMachine::AddShadowPass() -> void {
    auto pass = std::make_unique<BeStandardShadowPass>(this);
    _sequence.Passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddGeometryPass() -> void {
    be_assert(!_gbufferTargets.empty(), "No G-buffer targets declared before AddGeometryPass");
    be_assert(_depthTarget, "No depth target declared before AddGeometryPass");

    auto pass = std::make_unique<BeStandardGeometryPass>(this, _gbufferTargets, _depthTarget);
    _sequence.Passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddLightingPass(const std::string& outputName) -> void {
    be_assert(!_gbufferTargets.empty(), "No G-buffer targets declared before AddLightingPass");
    be_assert(_depthTarget, "No depth target declared before AddLightingPass");

    auto output = GetRenderTexture(outputName);
    be_assert(output, "AddLightingPass: output texture not found: " + outputName);

    auto pass = std::make_unique<BeStandardLightingPass>(this, _gbufferTargets, _depthTarget, _irradianceCubemap, _prefilteredCubemap, _brdfLutTexture, output);
    _sequence.Passes.push_back(std::move(pass));
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

    auto bloomTexture = DeclareTextureTarget("__standard_bloom", input->Format, false, mipCount);

    if (!dirtTexture) {
        dirtTexture = BeShaderLibrary::GetDefaultTexture("black").lock();
    }

    auto pass = std::make_unique<BeStandardBloomPass>(this, input, bloomTexture, output, dirtTexture, mipCount);
    _sequence.Passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddFullscreenPass(
    raw_ptr<BeShader> shader,
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
    _sequence.Passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddTonemapperPass(const std::string& inputName, const std::string& outputName) -> void {
    const auto shader = BeShaderLibrary::GetShader("tonemapper");
    be_assert(shader, "AddTonemapperPass: tonemapper shader not found");

    const auto& scheme = BeShaderLibrary::GetShaderScheme(*shader, "main");
    _tonemapperMaterial = BeMaterial::Create(scheme);
    _tonemapperMaterial->SetTexture("HDRInput", GetRenderTexture(inputName));

    AddFullscreenPass(shader, _tonemapperMaterial, { outputName });
}

auto BeStandardRenderMachine::AddBackbufferPass(const std::string& inputName) -> void {
    auto input = GetRenderTexture(inputName);
    be_assert(input, "AddBackbufferPass: input texture not found: " + inputName);

    auto pass = std::make_unique<BeStandardBackbufferPass>(this, input, _depthTarget);
    _sequence.Passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddPass(std::unique_ptr<BeRenderPass> pass) -> void {
    _sequence.Passes.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddEnvironmentBakePass(std::shared_ptr<BeTexture> equirect, uint32_t cubemapSize) -> void {
    _envCubemap = BeTexture::Create("__standard_env_cubemap")
        .SetUsage(SenTextureUsage::ColorTarget | SenTextureUsage::Sampled)
        .SetFormat(SenFormat::RGBA16_Float)
        .SetCubemap(true)
        .SetSize(cubemapSize, cubemapSize)
        .Build();

    _irradianceCubemap = BeTexture::Create("__standard_irradiance_cubemap")
        .SetUsage(SenTextureUsage::ColorTarget | SenTextureUsage::Sampled)
        .SetFormat(SenFormat::RGBA16_Float)
        .SetCubemap(true)
        .SetSize(32, 32)
        .Build();

    uint32_t prefilteredSize = cubemapSize / 4;
    uint32_t mipCount = 0;
    uint32_t s = prefilteredSize;
    while (s >= 1) {
        ++mipCount;
        s /= 2;
    }
    _prefilteredCubemap = BeTexture::Create("__standard_prefiltered_cubemap")
        .SetUsage(SenTextureUsage::ColorTarget | SenTextureUsage::Sampled)
        .SetFormat(SenFormat::RGBA16_Float)
        .SetCubemap(true)
        .SetSize(prefilteredSize, prefilteredSize)
        .SetMips(mipCount)
        .Build();

    _brdfLutTexture = BeTexture::Create("__standard_brdf_lut")
        .SetUsage(SenTextureUsage::ColorTarget | SenTextureUsage::Sampled)
        .SetFormat(SenFormat::RG32_Float)
        .SetSize(512, 512)
        .Build();

    _environmentBakePass = std::make_unique<BeStandardEnvironmentBakePass>(this, std::move(equirect), _envCubemap, _irradianceCubemap, _prefilteredCubemap, _brdfLutTexture);
}

auto BeStandardRenderMachine::BakeEnvironment() -> void {
    be_assert(_environmentBakePass, "BakeEnvironment: AddEnvironmentBakePass was not called");
    auto renderer = _renderer.lock();
    _environmentBakePass->Initialise(*renderer);
    renderer->RenderOnce({ _environmentBakePass.get() });
}

auto BeStandardRenderMachine::AddSkyboxPass(const std::string& outputName) -> void {
    be_assert(_envCubemap, "AddSkyboxPass: AddEnvironmentBakePass must be called first");
    be_assert(_depthTarget, "AddSkyboxPass: no depth target declared");

    auto output = GetRenderTexture(outputName);
    be_assert(output, "AddSkyboxPass: output texture not found: " + outputName);

    _sequence.Passes.push_back(std::make_unique<BeStandardSkyboxPass>(this, _depthTarget, _envCubemap, output));
}

// =====================================================================================================================
// BeStandardRenderMachine -build
// =====================================================================================================================

auto BeStandardRenderMachine::ClearPasses() -> void {
    _sequence.Passes.clear();
    _environmentBakePass.reset();
    _envCubemap.reset();
    _irradianceCubemap.reset();
    _prefilteredCubemap.reset();
    _brdfLutTexture.reset();
}

auto BeStandardRenderMachine::InitialisePasses() -> void {
    auto renderer = _renderer.lock();
    for (auto& pass : _sequence.Passes)
        pass->Initialise(*renderer);
}

auto BeStandardRenderMachine::Activate() -> void {
    _renderer.lock()->SetSequence(&_sequence);
}


// =====================================================================================================================
// BeStandardRenderMachine -debug
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
// BeStandardRenderMachine -frame submission
// =====================================================================================================================

auto BeStandardRenderMachine::ClearFrame() -> void {
    _geometryEntries.clear();
    _sunLightEntries.clear();
    _pointLightEntries.clear();

    if (_tonemapperMaterial) {
        _tonemapperMaterial->SetFloat1("Exposure", Settings.Tonemapper.Exposure);
        _tonemapperMaterial->SetFloat1("Contrast", Settings.Tonemapper.Contrast);
    }
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

auto BeStandardRenderMachine::EnsureShadowArrays() -> void {
    const uint32_t dirRes = Settings.Shadow.DirectionalResolution;
    const uint32_t dirCount = Settings.Shadow.MaxDirectional;
    if (dirCount > 0
        && (!_directionalShadowArray || _directionalShadowArray->Width != dirRes || _directionalShadowArray->ArrayLength != dirCount)) {
        _directionalShadowArray = BeTexture::Create("__srm_directional_shadow_array")
            .SetUsage(SenTextureUsage::DepthTarget | SenTextureUsage::Sampled)
            .SetFormat(SenFormat::Depth32)
            .SetSize(dirRes, dirRes)
            .SetArrayLength(dirCount)
            .Build();
    }

    const uint32_t pointRes = Settings.Shadow.PointResolution;
    const uint32_t pointCount = Settings.Shadow.MaxPoint;
    if (pointCount > 0
        && (!_pointShadowArray || _pointShadowArray->Width != pointRes || _pointShadowArray->ArrayLength != pointCount)) {
        _pointShadowArray = BeTexture::Create("__srm_point_shadow_array")
            .SetUsage(SenTextureUsage::DepthTarget | SenTextureUsage::Sampled)
            .SetFormat(SenFormat::Depth32)
            .SetCubemap(true)
            .SetSize(pointRes, pointRes)
            .SetArrayLength(pointCount)
            .Build();
    }
}

// =====================================================================================================================
// BeStandardRenderMachine -asset loading
// =====================================================================================================================

auto BeStandardRenderMachine::LoadProp(
    const std::filesystem::path& modelPath,
    raw_ptr<BeShader> shader,
    BeSRMLightingModel model
) -> std::shared_ptr<BeProp> {

    BeAssimpImporter::MaterialExtractFunction materialExtractFunction;

    const auto extractDrawState = [this, shader](aiMaterial const* mat, const BePropSurface& surface) -> void {
        int twoSided = 0;
        mat->Get(AI_MATKEY_TWOSIDED, twoSided);
        SenCull cull = twoSided != 0 ? SenCull::None : SenCull::Back;
        SetDrawState(surface, BeDrawState::Create(*shader).SetCull(cull).Build());
    };

    if (model == BeSRMLightingModel::PBR) {
        materialExtractFunction = [shader, extractDrawState](aiMaterial const* mat, aiScene const* scene, const std::filesystem::path& parentPath, const BePropSurface& surface) -> std::shared_ptr<BeMaterial> {
            extractDrawState(mat, surface);

            const auto scheme = BeShaderLibrary::GetShaderScheme(*shader, "geometry-main");
            auto material = BeMaterial::Create(scheme);

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
        materialExtractFunction = [shader, extractDrawState](aiMaterial const* mat, aiScene const* scene, const std::filesystem::path& parentPath, const BePropSurface& surface) -> std::shared_ptr<BeMaterial> {
            extractDrawState(mat, surface);

            const auto scheme = BeShaderLibrary::GetShaderScheme(*shader, "geometry-main");
            auto material = BeMaterial::Create(scheme);

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
    return importer.LoadProp(modelPath, materialExtractFunction);
}

auto BeStandardRenderMachine::CreateProp(
    std::shared_ptr<BeMeshData> mesh,
    raw_ptr<BeShader> shader,
    const std::string& schemeLink
) -> std::shared_ptr<BeProp> {
    auto prop = std::make_shared<BeProp>();
    prop->MeshData = std::move(mesh);
    prop->Mesh = BeBackend::CreateMesh(*prop->MeshData);

    const auto& scheme = BeShaderLibrary::GetShaderScheme(*shader, schemeLink);
    const auto state = BeDrawState::Create(*shader).Build();
    for (size_t i = 0; i < prop->MeshData->Submeshes.size(); ++i) {
        auto& surface = prop->Surfaces.emplace_back();
        surface.Submesh = static_cast<uint32_t>(i);
        surface.Material = BeMaterial::Create(scheme);
        SetDrawState(surface, state);
    }

    return prop;
}


// =====================================================================================================================
// BeStandardRenderMachine -draw states
// =====================================================================================================================

auto BeStandardRenderMachine::ClearDrawStates() -> void {
    _surfaceDrawStates.clear();
}

auto BeStandardRenderMachine::SetDrawState(const BePropSurface& surface, BeDrawState state) -> void {
    _surfaceDrawStates[surface.Id] = std::move(state);
}

auto BeStandardRenderMachine::GetDrawState(const BePropSurface& surface) const -> const BeDrawState& {
    const auto it = _surfaceDrawStates.find(surface.Id);
    be_assert(it != _surfaceDrawStates.end(), "BeStandardRenderMachine::GetDrawState: surface was never registered", surface.Id);
    return it->second;
}

