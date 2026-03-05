#include "BeBRPSubmissionBuffer.h"

#include "Utils.h"
#include "umbrellas/include-libassert.h"
#include <sen-rhi/dx11/SenDx11Backend.h>


auto BeBRPGeometryEntry::CalculateModelMatrix(glm::vec3 pos, glm::quat rot, glm::vec3 scale) -> glm::mat4 {
    const glm::mat4x4 modelMatrix =
        glm::translate(glm::mat4(1.0f), pos) *
        glm::mat4_cast(rot) *
        glm::scale(glm::mat4(1.0f), scale);
    return modelMatrix;
}

auto BeBRPSunLightEntry::CalculateViewProj(glm::vec3 direction, float shadowCameraDistance, float shadowMapWorldSize,
                                           float shadowNearPlane, float shadowFarPlane) -> glm::mat4 {
    const float halfSize = shadowMapWorldSize * 0.5f;
    const glm::mat4 lightOrtho = glm::orthoLH_ZO(-halfSize, halfSize, -halfSize, halfSize, shadowNearPlane, shadowFarPlane);
    const glm::vec3 lightPos = -direction * shadowCameraDistance;
    const glm::mat4 lightView = glm::lookAtLH(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    return lightOrtho * lightView;
}

auto BeBRPSubmissionBuffer::Init(const ComPtr<ID3D11Device>& device) -> void {
    _device = device;
}

auto BeBRPSubmissionBuffer::ClearEntries() -> void {
    _geometryEntries.clear();
    _sunLightEntries.clear();
    _pointLightEntries.clear();
}

auto BeBRPSubmissionBuffer::SubmitGeometry(const BeBRPGeometryEntry& entry) -> void {
    _geometryEntries.push_back(entry);
}

auto BeBRPSubmissionBuffer::SubmitSunLight(const BeBRPSunLightEntry& entry) -> void {
    _sunLightEntries.push_back(entry);
}

auto BeBRPSubmissionBuffer::SubmitPointLight(const BeBRPPointLightEntry& entry) -> void {
    _pointLightEntries.push_back(entry);
}

auto BeBRPSubmissionBuffer::GetGeometryEntries() const -> const std::vector<BeBRPGeometryEntry>& {
    return _geometryEntries;
}

auto BeBRPSubmissionBuffer::GetSunLightEntries() const -> const std::vector<BeBRPSunLightEntry>& {
    return _sunLightEntries;
}

auto BeBRPSubmissionBuffer::GetPointLightEntries() const -> const std::vector<BeBRPPointLightEntry>& {
    return _pointLightEntries;
}

auto BeBRPSubmissionBuffer::RegisterMesh(const std::shared_ptr<BeMesh>& mesh) -> void {
    _registeredMeshes.push_back(mesh);
}

auto BeBRPSubmissionBuffer::BakeMeshes() -> void {
    be_assert(_device, "No device given. Submission buffer is probably uninitialized. Call Init.");

    //vbo + ibo
    size_t totalVerticesNumber = 0;
    size_t totalIndicesNumber = 0;
    for (const auto& mesh : _registeredMeshes) {
        totalVerticesNumber += mesh->Vertices.size();
        totalIndicesNumber += mesh->Indices.size();
    }

    std::vector<BeFullVertex> fullVertices;
    std::vector<uint32_t> indices;
    fullVertices.reserve(totalVerticesNumber);
    indices.reserve(totalIndicesNumber);
    for (auto& mesh : _registeredMeshes) {
        if (_meshSlices.contains(mesh.get()))
            continue;

        const auto vertexBase = static_cast<int32_t>(fullVertices.size());
        const auto indexBase = static_cast<uint32_t>(indices.size());

        fullVertices.insert(fullVertices.end(), mesh->Vertices.begin(), mesh->Vertices.end());
        indices.insert(indices.end(), mesh->Indices.begin(), mesh->Indices.end());

        auto& slices = _meshSlices[mesh.get()];
        for (const auto& slice : mesh->Slices) {
            slices.push_back({
                .IndexCount = slice.IndexCount,
                .StartIndexLocation = slice.StartIndexLocation + indexBase,
                .BaseVertexLocation = slice.BaseVertexLocation + vertexBase,
            });
        }
    }

    _sharedVertexBuffer = SenDx11Backend::Get().CreateBuffer(_device, {
        .usage  = SenBufferUsage::Vertex,
        .access = SenBufferAccess::Immutable,
        .size   = static_cast<uint32_t>(fullVertices.size() * sizeof(BeFullVertex)),
        .data   = fullVertices.data(),
    });

    _sharedIndexBuffer = SenDx11Backend::Get().CreateBuffer(_device, {
        .usage  = SenBufferUsage::Index,
        .access = SenBufferAccess::Immutable,
        .size   = static_cast<uint32_t>(indices.size() * sizeof(uint32_t)),
        .data   = indices.data(),
    });
}


