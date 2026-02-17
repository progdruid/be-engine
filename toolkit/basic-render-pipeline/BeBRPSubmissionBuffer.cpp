#include "BeBRPSubmissionBuffer.h"

#include "BeModel.h"
#include "Utils.h"
#include "umbrellas/include-libassert.h"


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

auto BeBRPSubmissionBuffer::RegisterModel(const std::shared_ptr<BeModel>& model) -> void {
    _registeredModels.push_back(model);
}

auto BeBRPSubmissionBuffer::BakeModels() -> void {
    be_assert(_device, "No device given. Submission buffer is probably uninitialized. Call Init.");

    //vbo + ibo
    size_t totalVerticesNumber = 0;
    size_t totalIndicesNumber = 0;
    size_t totalDrawSlices = 0;
    for (const auto& model : _registeredModels) {
        totalVerticesNumber += model->FullVertices.size();
        totalIndicesNumber += model->Indices.size();
        totalDrawSlices += model->DrawSlices.size();
    }

    std::vector<BeFullVertex> fullVertices;
    std::vector<uint32_t> indices;
    fullVertices.reserve(totalVerticesNumber);
    indices.reserve(totalIndicesNumber);
    for (auto& model : _registeredModels) {
        fullVertices.insert(fullVertices.end(), model->FullVertices.begin(), model->FullVertices.end());
        indices.insert(indices.end(), model->Indices.begin(), model->Indices.end());

        auto & drawSlices = _modelDrawSlices[model.get()];
        
        for (auto slice : model->DrawSlices) {
            slice.BaseVertexLocation += static_cast<int32_t>(fullVertices.size() - model->FullVertices.size());
            slice.StartIndexLocation += static_cast<uint32_t>(indices.size() - model->Indices.size());
            
            drawSlices.push_back(slice);
        }
    }
    
    D3D11_BUFFER_DESC vertexBufferDescriptor = {};
    vertexBufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDescriptor.ByteWidth = static_cast<UINT>(fullVertices.size() * sizeof(BeFullVertex));
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = fullVertices.data();
    Utils::Check << _device->CreateBuffer(&vertexBufferDescriptor, &vertexData, &_sharedVertexBuffer);
    
    D3D11_BUFFER_DESC indexBufferDescriptor = {};
    indexBufferDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDescriptor.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();
    Utils::Check << _device->CreateBuffer(&indexBufferDescriptor, &indexData, &_sharedIndexBuffer);
}


