#include "basic-render-pipeline/BeBRPSubmissionBuffer.h"
#include "platform/dx11/BeBRPSubmissionBufferImpl.h"
#include "platform/dx11/BeRendererImpl.h"
#include "platform/dx11/DxUtils.h"

#include "BeRenderer.h"
#include "BeModel.h"

auto BeBRPSubmissionBuffer::Init(BeRenderer& renderer) -> void {
    _impl = std::make_unique<BeBRPSubmissionBufferImpl>();
    _impl->device = renderer.GetPlatformImpl()->device;
}

auto BeBRPSubmissionBuffer::BakeModels() -> void {
    be_assert(_impl && _impl->device, "No device given. Submission buffer is probably uninitialized. Call Init.");

    size_t totalVerticesNumber = 0;
    size_t totalIndicesNumber = 0;
    for (const auto& model : _registeredModels) {
        totalVerticesNumber += model->FullVertices.size();
        totalIndicesNumber += model->Indices.size();
    }

    std::vector<BeFullVertex> fullVertices;
    std::vector<uint32_t> indices;
    fullVertices.reserve(totalVerticesNumber);
    indices.reserve(totalIndicesNumber);
    for (auto& model : _registeredModels) {
        fullVertices.insert(fullVertices.end(), model->FullVertices.begin(), model->FullVertices.end());
        indices.insert(indices.end(), model->Indices.begin(), model->Indices.end());

        auto& drawSlices = _modelDrawSlices[model.get()];
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
    DxUtils::Check << _impl->device->CreateBuffer(&vertexBufferDescriptor, &vertexData, &_impl->sharedVertexBuffer);

    D3D11_BUFFER_DESC indexBufferDescriptor = {};
    indexBufferDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDescriptor.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();
    DxUtils::Check << _impl->device->CreateBuffer(&indexBufferDescriptor, &indexData, &_impl->sharedIndexBuffer);
}
