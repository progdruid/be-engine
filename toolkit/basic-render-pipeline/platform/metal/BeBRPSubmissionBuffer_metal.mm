#include "basic-render-pipeline/BeBRPSubmissionBuffer.h"
#include "platform/metal/BeBRPSubmissionBufferImpl.h"
#include "platform/metal/BeRendererImpl.h"
#include "platform/metal/MetalUtils.h"

#include "BeRenderer.h"
#include "BeModel.h"

#import <Metal/Metal.h>

BeBRPSubmissionBuffer::BeBRPSubmissionBuffer() = default;
BeBRPSubmissionBuffer::~BeBRPSubmissionBuffer() = default;

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

    _impl->sharedVertexBuffer = [_impl->device newBufferWithBytes:fullVertices.data()
                                                           length:fullVertices.size() * sizeof(BeFullVertex)
                                                          options:MTLResourceStorageModeShared];

    _impl->sharedIndexBuffer = [_impl->device newBufferWithBytes:indices.data()
                                                          length:indices.size() * sizeof(uint32_t)
                                                         options:MTLResourceStorageModeShared];
}
