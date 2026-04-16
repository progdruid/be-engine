#include "BeStandardRenderMachine.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardShadowPass.h"
#include "standard-render-machine/BeStandardGeometryPass.h"
#include "standard-render-machine/BeStandardLightingPass.h"
#include "standard-render-machine/BeStandardBloomPass.h"
#include "standard-render-machine/BeStandardFullscreenEffectPass.h"
#include "standard-render-machine/BeStandardBackbufferPass.h"

// =====================================================================================================================
// BeStandardGeometryEntry
// =====================================================================================================================

auto BeSRMGeometryEntry::CalculateModelMatrix(glm::vec3 pos, glm::quat rot, glm::vec3 scale) -> glm::mat4 {
    return
        glm::translate(glm::mat4(1.0f), pos) *
        glm::mat4_cast(rot) *
        glm::scale(glm::mat4(1.0f), scale);
}

// =====================================================================================================================
// BeStandardSunLightEntry
// =====================================================================================================================

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

// =====================================================================================================================
// BeStandardRenderMachine — lifetime
// =====================================================================================================================

BeStandardRenderMachine::BeStandardRenderMachine(BeRenderer& renderer, uint32_t width, uint32_t height)
    : _renderer(&renderer), _width(width), _height(height) {}

// =====================================================================================================================
// BeStandardRenderMachine — texture registry
// =====================================================================================================================

auto BeStandardRenderMachine::DeclareGBufferTarget(const std::string& name, SenFormat format) -> std::shared_ptr<BeTexture> {
    auto texture = BeTexture::Create(name)
        .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
        .SetFormat(format)
        .SetSize(_width, _height)
        .Build();

    _textureRegistry.push_back(TextureEntry{ name, texture, true, false });
    _gbufferTargets.push_back(texture);
    return texture;
}

auto BeStandardRenderMachine::DeclareDepth(const std::string& name, SenFormat format) -> std::shared_ptr<BeTexture> {
    auto texture = BeTexture::Create(name)
        .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
        .SetFormat(format)
        .SetSize(_width, _height)
        .Build();

    _textureRegistry.push_back({ name, texture, false, true });
    _depthTarget = texture;
    return texture;
}

auto BeStandardRenderMachine::DeclareTexture(const std::string& name, SenFormat format, float sizeMultiplier) -> std::shared_ptr<BeTexture> {
    const auto w = static_cast<uint32_t>(static_cast<float>(_width)  * sizeMultiplier);
    const auto h = static_cast<uint32_t>(static_cast<float>(_height) * sizeMultiplier);

    auto texture = BeTexture::Create(name)
        .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
        .SetFormat(format)
        .SetSize(w, h)
        .Build();

    _textureRegistry.push_back({ name, texture, false, false });
    return texture;
}

auto BeStandardRenderMachine::GetTexture(const std::string& name) const -> std::shared_ptr<BeTexture> {
    for (const auto& entry : _textureRegistry)
        if (entry.Name == name)
            return entry.Texture;
    return nullptr;
}

// =====================================================================================================================
// BeStandardRenderMachine — pass builders
// =====================================================================================================================

auto BeStandardRenderMachine::AddShadowPass() -> void {
    auto pass = std::make_unique<BeStandardShadowPass>(this);
    _passOrder.push_back(pass.get());
    _ownedPasses.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddGeometryPass() -> void {
    be_assert(!_gbufferTargets.empty(), "No G-buffer targets declared before AddGeometryPass");
    be_assert(_depthTarget, "No depth target declared before AddGeometryPass");

    auto pass = std::make_unique<BeStandardGeometryPass>(this, _gbufferTargets, _depthTarget);
    _passOrder.push_back(pass.get());
    _ownedPasses.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddLightingPass(const std::string& outputName) -> void {
    be_assert(!_gbufferTargets.empty(), "No G-buffer targets declared before AddLightingPass");
    be_assert(_depthTarget, "No depth target declared before AddLightingPass");

    auto output = GetTexture(outputName);
    be_assert(output, "AddLightingPass: output texture not found: " + outputName);

    auto pass = std::make_unique<BeStandardLightingPass>(this, _gbufferTargets, _depthTarget, output);
    _passOrder.push_back(pass.get());
    _ownedPasses.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddBloomPass(
    uint32_t mipCount,
    const std::string& inputName,
    const std::string& outputName,
    std::shared_ptr<BeTexture> dirtTexture
) -> void {
    auto input  = GetTexture(inputName);
    auto output = GetTexture(outputName);
    be_assert(input,  "AddBloomPass: input texture not found: "  + inputName);
    be_assert(output, "AddBloomPass: output texture not found: " + outputName);

    std::vector<std::shared_ptr<BeTexture>> mipTextures;
    mipTextures.reserve(mipCount);
    for (uint32_t i = 0; i < mipCount; ++i) {
        const float mult = 1.0f / static_cast<float>(1u << i);
        mipTextures.push_back(DeclareTexture("__standard_bloom_mip_" + std::to_string(i), input->Format, mult));
    }

    if (!dirtTexture)
        dirtTexture = BeAssetRegistry::GetTexture("black").lock();

    auto pass = std::make_unique<BeStandardBloomPass>(this, input, std::move(mipTextures), output, dirtTexture, mipCount);
    _passOrder.push_back(pass.get());
    _ownedPasses.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddFullscreenPass(
    std::weak_ptr<BeShader> shader,
    std::shared_ptr<BeMaterial> material,
    const std::vector<std::string>& outputNames
) -> void {
    std::vector<std::shared_ptr<BeTexture>> outputs;
    outputs.reserve(outputNames.size());
    for (const auto& name : outputNames) {
        auto tex = GetTexture(name);
        be_assert(tex, "AddFullscreenPass: output texture not found: " + name);
        outputs.push_back(tex);
    }

    auto pass = std::make_unique<BeStandardFullscreenEffectPass>(this, shader, material, std::move(outputs));
    _passOrder.push_back(pass.get());
    _ownedPasses.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddBackbufferPass(const std::string& inputName, glm::vec3 clearColor) -> void {
    auto input = GetTexture(inputName);
    be_assert(input, "AddBackbufferPass: input texture not found: " + inputName);

    auto pass = std::make_unique<BeStandardBackbufferPass>(this, input, clearColor);
    _passOrder.push_back(pass.get());
    _ownedPasses.push_back(std::move(pass));
}

auto BeStandardRenderMachine::AddPass(BeRenderPass* pass) -> void {
    _passOrder.push_back(pass);
}

// =====================================================================================================================
// BeStandardRenderMachine — build
// =====================================================================================================================

auto BeStandardRenderMachine::Build() -> void {
    _renderer->ClearPasses();
    for (auto* pass : _passOrder)
        _renderer->AddRenderPass(pass);
    _renderer->InitialisePasses();
}

// =====================================================================================================================
// BeStandardRenderMachine — frame submission
// =====================================================================================================================

auto BeStandardRenderMachine::Clear() -> void {
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

auto BeStandardRenderMachine::AcquireNewObjectMaterial() -> std::shared_ptr<BeMaterial> {
    if (_objectMaterialCursor >= _objectMaterialPool.size())
        _objectMaterialPool.push_back(BeMaterial::Create("object-material-for-geometry-pass", true));
    return _objectMaterialPool[_objectMaterialCursor++];
}
