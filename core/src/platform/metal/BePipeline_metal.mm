#include "BePipeline.h"
#include "BePipelineImpl.h"
#include "BeRendererImpl.h"
#include "BeTextureImpl.h"
#include "BeShaderImpl.h"
#include "BeMaterialImpl.h"
#include "MetalUtils.h"

#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeMaterial.h"
#include "BeModel.h"
#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeBRPSubmissionBufferImpl.h"

#import <Metal/Metal.h>

#include <cassert>
#include <umbrellas/include-glm.h>
#include <umbrellas/include-libassert.h>

struct BeSamplerImpl {
    id<MTLSamplerState> samplerState;
};

auto BePipeline::Create(BeRenderer& renderer) -> std::shared_ptr<BePipeline> {
    auto pipeline = std::shared_ptr<BePipeline>(new BePipeline());
    pipeline->_impl = std::make_unique<BePipelineImpl>();
    pipeline->_impl->device = renderer.GetPlatformImpl()->device;
    pipeline->_impl->rendererImpl = renderer.GetPlatformImpl();
    return pipeline;
}

static auto GetOrCreatePipelineState(
    BePipelineImpl* impl,
    BeRendererImpl* ri,
    BeShaderImpl* si,
    const std::vector<id<MTLTexture>>& colorAttachments,
    id<MTLTexture> depthAttachment
) -> id<MTLRenderPipelineState> {
    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = si->vertexFunction;
    desc.fragmentFunction = si->pixelFunction;
    if (si->vertexDescriptor) {
        desc.vertexDescriptor = si->vertexDescriptor;
    }

    for (NSUInteger i = 0; i < colorAttachments.size(); i++) {
        desc.colorAttachments[i].pixelFormat = [colorAttachments[i] pixelFormat];
    }

    if (depthAttachment) {
        desc.depthAttachmentPixelFormat = [depthAttachment pixelFormat];
    }

    NSError* error = nil;
    id<MTLRenderPipelineState> state = [impl->device newRenderPipelineStateWithDescriptor:desc error:&error];
    be_assert(state != nil, "Failed to create pipeline state: " +
        std::string([[error localizedDescription] UTF8String]));
    return state;
}

static auto EnsureEncoder(
    BeRendererImpl* ri,
    const std::vector<id<MTLTexture>>& colorAttachments,
    id<MTLTexture> depthAttachment,
    bool clearRTVs
) -> void {
    if (ri->currentEncoder) {
        [ri->currentEncoder endEncoding];
        ri->currentEncoder = nil;
    }

    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    for (NSUInteger i = 0; i < colorAttachments.size(); i++) {
        passDesc.colorAttachments[i].texture = colorAttachments[i];
        passDesc.colorAttachments[i].loadAction = clearRTVs ? MTLLoadActionClear : MTLLoadActionLoad;
        passDesc.colorAttachments[i].storeAction = MTLStoreActionStore;
        passDesc.colorAttachments[i].clearColor = MTLClearColorMake(0, 0, 0, 0);
    }
    if (depthAttachment) {
        passDesc.depthAttachment.texture = depthAttachment;
        passDesc.depthAttachment.loadAction = MTLLoadActionClear;
        passDesc.depthAttachment.storeAction = MTLStoreActionStore;
        passDesc.depthAttachment.clearDepth = 1.0;
    }

    ri->currentEncoder = [ri->currentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
    [ri->currentEncoder setDepthStencilState:ri->defaultDepthStencilState];
    [ri->currentEncoder setCullMode:ri->currentCullMode];
}

auto BePipeline::BindShader(const std::shared_ptr<BeShader>& shader, BeShaderType shaderType) -> void {
    assert(_boundShaderType == BeShaderType::None);
    assert(_boundShader == nullptr);
    assert(shader->Topology != BeTopology::Undefined);

    _boundShaderType = shader->ShaderType & shaderType;
    _boundShader = shader;
}

auto BePipeline::BindMaterialAutomatic(const std::shared_ptr<BeMaterial>& material) -> void {
    assert(_boundShader);
    const uint8_t slot = _boundShader->GetMaterialSlotByScheme(material->GetSchemeName());
    BindMaterialManual(material, slot);
}

auto BePipeline::BindMaterialManual(const std::shared_ptr<BeMaterial>& material, const uint8_t materialSlot) -> void {
    auto* ri = _impl->rendererImpl;
    auto encoder = ri->currentEncoder;
    if (!encoder) return;

    auto* matImpl = material->GetPlatformImpl();
    if (matImpl && matImpl->cbuffer) {
        material->UpdatePlatformBuffer();
        [encoder setVertexBuffer:matImpl->cbuffer offset:0 atIndex:materialSlot];
        [encoder setFragmentBuffer:matImpl->cbuffer offset:0 atIndex:materialSlot];
    }

    const auto& textureSlots = material->GetTexturePairs();
    for (const auto& [texture, slot] : textureSlots | std::views::values) {
        auto* texImpl = texture->GetPlatformImpl();
        if (texImpl->texture) {
            [encoder setFragmentTexture:texImpl->texture atIndex:slot];
            [encoder setVertexTexture:texImpl->texture atIndex:slot];
        }
    }

    const auto& samplerSlots = material->GetSamplerPairsInternal();
    for (auto& [sampler, slot] : samplerSlots | std::views::values) {
        [encoder setFragmentSamplerState:sampler->samplerState atIndex:slot];
        [encoder setVertexSamplerState:sampler->samplerState atIndex:slot];
    }
}

auto BePipeline::Clear() -> void {
    assert(_boundShaderType != BeShaderType::None);
    assert(_boundShader != nullptr);
    _boundShaderType = BeShaderType::None;
    _boundShader.reset();
    _boundShader = nullptr;
}

auto BePipeline::ClearCache() -> void {
    _impl->pipelineStateCache.clear();
}

auto BePipeline::BindMaterialTextures(const BeMaterial& material) -> void {
    auto* ri = _impl->rendererImpl;
    auto encoder = ri->currentEncoder;
    if (!encoder) return;

    const auto& textureSlots = material.GetTexturePairs();
    for (const auto& [texture, slot] : textureSlots | std::views::values) {
        auto* texImpl = texture->GetPlatformImpl();
        if (texImpl->texture) {
            [encoder setFragmentTexture:texImpl->texture atIndex:slot];
            [encoder setVertexTexture:texImpl->texture atIndex:slot];
        }
    }
}

auto BePipeline::BindTargets(
    const std::vector<std::weak_ptr<BeTexture>>& renderTargets,
    const BeTexture* depthTarget,
    bool clearRTVs
) const -> void {
    auto* ri = _impl->rendererImpl;

    std::vector<id<MTLTexture>> colorAttachments;
    colorAttachments.reserve(renderTargets.size());
    for (const auto& rt : renderTargets) {
        auto* texImpl = rt.lock()->GetPlatformImpl();
        colorAttachments.push_back(texImpl->mipRenderTargetViews.empty() ? texImpl->texture : texImpl->mipRenderTargetViews[0]);
    }

    id<MTLTexture> depthAttachment = nil;
    if (depthTarget) {
        depthAttachment = depthTarget->GetPlatformImpl()->depthTexture;
    }

    EnsureEncoder(ri, colorAttachments, depthAttachment, clearRTVs);

    if (_boundShader) {
        auto* si = _boundShader->GetPlatformImpl();
        auto state = GetOrCreatePipelineState(_impl.get(), ri, si, colorAttachments, depthAttachment);
        [ri->currentEncoder setRenderPipelineState:state];

        [ri->currentEncoder setVertexBuffer:ri->uniformBuffer offset:0 atIndex:0];
        [ri->currentEncoder setFragmentBuffer:ri->uniformBuffer offset:0 atIndex:0];
    }
}

auto BePipeline::ClearTargets() const -> void {
    auto* ri = _impl->rendererImpl;
    if (ri->currentEncoder) {
        [ri->currentEncoder endEncoding];
        ri->currentEncoder = nil;
    }
}

auto BePipeline::ResetTarget(const std::shared_ptr<BeTexture>& texture) const -> void {
    be_assert(
        HasAny(texture->BindFlags, BeBindFlags::RenderTarget),
        "trying to reset a texture that is not a render target."
    );
}

auto BePipeline::Draw(uint32_t vertexCount, uint32_t startVertexLocation) const -> void {
    auto encoder = _impl->rendererImpl->currentEncoder;
    if (!encoder) return;
    [encoder drawPrimitives:MetalUtils::ToMTLPrimitiveType(_boundShader ? _boundShader->Topology : BeTopology::TriangleList)
                vertexStart:startVertexLocation
                vertexCount:vertexCount];
}

auto BePipeline::DrawSlice(const BeDrawSlice& slice) const -> void {
    DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
}

auto BePipeline::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) const -> void {
    auto encoder = _impl->rendererImpl->currentEncoder;
    if (!encoder) return;

    auto* bufImpl = _impl->rendererImpl;
    [encoder drawIndexedPrimitives:MetalUtils::ToMTLPrimitiveType(_boundShader ? _boundShader->Topology : BeTopology::TriangleList)
                        indexCount:indexCount
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:nil
                 indexBufferOffset:startIndex * sizeof(uint32_t)
                     instanceCount:1
                        baseVertex:baseVertex
                      baseInstance:0];
}

auto BePipeline::SetViewport(const BeViewport& viewport) const -> void {
    auto encoder = _impl->rendererImpl->currentEncoder;
    if (!encoder) return;
    MTLViewport vp = {};
    vp.originX = viewport.TopLeftX;
    vp.originY = viewport.TopLeftY;
    vp.width = viewport.Width;
    vp.height = viewport.Height;
    vp.znear = viewport.MinDepth;
    vp.zfar = viewport.MaxDepth;
    [encoder setViewport:vp];
}

auto BePipeline::GetViewport() const -> BeViewport {
    BeViewport vp;
    return vp;
}

auto BePipeline::SetCullMode(BeCullMode mode) const -> void {
    auto* ri = _impl->rendererImpl;
    switch (mode) {
        case BeCullMode::None:
            ri->currentCullMode = MTLCullModeNone;
            break;
        case BeCullMode::Front:
            ri->currentCullMode = MTLCullModeFront;
            break;
        case BeCullMode::Back:
        default:
            ri->currentCullMode = MTLCullModeBack;
            break;
    }
    if (ri->currentEncoder) {
        [ri->currentEncoder setCullMode:ri->currentCullMode];
    }
}

auto BePipeline::BindMeshBuffers(const BeBRPSubmissionBuffer& submissionBuffer) const -> void {
    auto encoder = _impl->rendererImpl->currentEncoder;
    if (!encoder) return;
    auto* bufImpl = submissionBuffer.GetPlatformImpl();
    [encoder setVertexBuffer:bufImpl->sharedVertexBuffer offset:0 atIndex:30];
}

auto BePipeline::UnbindMeshBuffers() const -> void {
}

auto BePipeline::SetDepthOnlyTarget(BeTexture* depthTexture) const -> void {
    auto* ri = _impl->rendererImpl;
    auto* texImpl = depthTexture->GetPlatformImpl();
    std::vector<id<MTLTexture>> empty;
    EnsureEncoder(ri, empty, texImpl->depthTexture, false);
}

auto BePipeline::SetCubemapDepthTarget(BeTexture* cubemapTexture, uint32_t face) const -> void {
    auto* ri = _impl->rendererImpl;
    auto* texImpl = cubemapTexture->GetPlatformImpl();
    std::vector<id<MTLTexture>> empty;
    EnsureEncoder(ri, empty, texImpl->cubemapDepthViews[face], false);
}

auto BePipeline::ClearDepthTarget(BeTexture* depthTexture) const -> void {
    auto* ri = _impl->rendererImpl;
    auto* texImpl = depthTexture->GetPlatformImpl();
    std::vector<id<MTLTexture>> empty;
    EnsureEncoder(ri, empty, texImpl->depthTexture, true);
    if (ri->currentEncoder) {
        [ri->currentEncoder endEncoding];
        ri->currentEncoder = nil;
    }
}

auto BePipeline::BindBackbuffer(const glm::vec4& clearColor) const -> void {
    auto* ri = _impl->rendererImpl;
    if (!ri->currentDrawable) return;

    if (ri->currentEncoder) {
        [ri->currentEncoder endEncoding];
        ri->currentEncoder = nil;
    }

    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = ri->currentDrawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake(clearColor.r, clearColor.g, clearColor.b, clearColor.a);

    ri->currentEncoder = [ri->currentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
    [ri->currentEncoder setDepthStencilState:ri->disabledDepthStencilState];

    if (_boundShader) {
        auto* si = _boundShader->GetPlatformImpl();
        MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
        desc.vertexFunction = si->vertexFunction;
        desc.fragmentFunction = si->pixelFunction;
        if (si->vertexDescriptor) desc.vertexDescriptor = si->vertexDescriptor;
        desc.colorAttachments[0].pixelFormat = ri->currentDrawable.texture.pixelFormat;

        NSError* error = nil;
        auto state = [_impl->device newRenderPipelineStateWithDescriptor:desc error:&error];
        [ri->currentEncoder setRenderPipelineState:state];
    }
}

auto BePipeline::UnbindBackbuffer() const -> void {
    auto* ri = _impl->rendererImpl;
    if (ri->currentEncoder) {
        [ri->currentEncoder endEncoding];
        ri->currentEncoder = nil;
    }
}

auto BePipeline::SetAdditiveBlending() const -> void {
}

auto BePipeline::ClearBlendState() const -> void {
}

auto BePipeline::UpdateMaterialBuffers(const std::shared_ptr<BeMaterial>& material) const -> void {
    material->UpdatePlatformBuffer();
}
