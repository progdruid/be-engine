#include "SenVulkanState.h"
#include "SenVulkanConvert.h"

#include <algorithm>
#include <umbrellas/include-libassert.h>

using namespace SenVk;


// ─── render pass ──────────────────────────────────────────────────────────────

auto SenCmd::BeginPass(SenCommandList list, const SenRenderPassDesc& desc) -> void {
    const VkCommandBuffer cmd = SenVk::LookupCommandList(list).Cmd;

    // Color attachments
    std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
    colorAttachments.reserve(desc.ColorAttachments.size());

    for (const auto& attachment : desc.ColorAttachments) {
        const VkImageView view = SenVk::LookupView(attachment.View).View;

        VkClearValue clearValue;
        clearValue.color = { attachment.ClearColor[0], attachment.ClearColor[1], attachment.ClearColor[2], attachment.ClearColor[3] };

        colorAttachments.push_back(VkRenderingAttachmentInfoKHR {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView   = view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = attachment.LoadOp == SenLoadOp::Clear    ? VK_ATTACHMENT_LOAD_OP_CLEAR     :
                           attachment.LoadOp == SenLoadOp::Load     ? VK_ATTACHMENT_LOAD_OP_LOAD      :
                                                                       VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = clearValue,
        });
    }

    // Depth attachment
    VkRenderingAttachmentInfoKHR depthAttachmentInfo {};
    bool hasDepth = desc.DepthAttachment.has_value();
    if (hasDepth) {
        const auto& depthAttach = desc.DepthAttachment.value();
        const VkImageView view = SenVk::LookupView(depthAttach.View).View;

        VkClearValue clearValue {};
        clearValue.depthStencil = { depthAttach.ClearDepth, depthAttach.ClearStencil };

        depthAttachmentInfo = VkRenderingAttachmentInfoKHR {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView   = view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp      = depthAttach.LoadOp == SenLoadOp::Clear    ? VK_ATTACHMENT_LOAD_OP_CLEAR     :
                           depthAttach.LoadOp == SenLoadOp::Load     ? VK_ATTACHMENT_LOAD_OP_LOAD      :
                                                                        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = clearValue,
        };
    }

    be_assert(
        desc.Viewport.Width > 0.f && desc.Viewport.Height > 0.f,
        "BeginPass: viewport width and height must be positive"
    );

    VkRenderingInfoKHR renderingInfo {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea           = {
            .offset = { int32_t(desc.Viewport.X), int32_t(desc.Viewport.Y) },
            .extent = { uint32_t(desc.Viewport.Width), uint32_t(desc.Viewport.Height) },
        },
        .layerCount           = 1,
        .colorAttachmentCount = uint32_t(colorAttachments.size()),
        .pColorAttachments    = colorAttachments.data(),
        .pDepthAttachment     = hasDepth ? &depthAttachmentInfo : nullptr,
    };

    vkCmdBeginRendering(cmd, &renderingInfo);

    // Flip viewport Y so NDC Y+ = up (matches DX11/GLM convention).
    // Vulkan default has Y+ = down in NDC; negative height reverses this.
    VkViewport vp {
        .x        = desc.Viewport.X,
        .y        = desc.Viewport.Y + desc.Viewport.Height,
        .width    = desc.Viewport.Width,
        .height   = -desc.Viewport.Height,
        .minDepth = desc.Viewport.MinDepth,
        .maxDepth = desc.Viewport.MaxDepth,
    };
    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D scissor {
        .offset = { int32_t(desc.Viewport.X), int32_t(desc.Viewport.Y) },
        .extent = { uint32_t(desc.Viewport.Width), uint32_t(desc.Viewport.Height) },
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

auto SenCmd::EndPass(SenCommandList list) -> void {
    vkCmdEndRendering(SenVk::LookupCommandList(list).Cmd);
}

auto SenCmd::CopyBuffer(SenCommandList list, SenBuffer src, uint32_t srcOffset, uint32_t size, SenBuffer dst, uint32_t dstOffset) -> void {
    const VkBufferCopy region {
        .srcOffset = srcOffset,
        .dstOffset = dstOffset,
        .size      = size,
    };
    vkCmdCopyBuffer(
        SenVk::LookupCommandList(list).Cmd,
        SenVk::LookupBuffer(src).Buffer,
        SenVk::LookupBuffer(dst).Buffer,
        1, &region
    );
}

auto SenCmd::CopyBufferToTexture(SenCommandList list, SenBuffer src, uint32_t srcOffset, SenTexture dst, uint32_t mip) -> void {
    auto& entry = SenVk::LookupTexture(dst);
    be_assert(mip < entry.MipLevels, "CopyBufferToTexture: mip out of range");

    const uint32_t mipWidth  = std::max(1u, entry.Width >> mip);
    const uint32_t mipHeight = std::max(1u, entry.Height >> mip);
    const uint32_t faceSize  = mipWidth * mipHeight * SenGetFormatBytes(SenVk::FromVkFormat(entry.Format));

    // Source is tightly packed, layers consecutive.
    std::vector<VkBufferImageCopy> regions(entry.LayerCount);
    for (uint32_t layer = 0; layer < entry.LayerCount; ++layer) {
        regions[layer] = {
            .bufferOffset      = VkDeviceSize(srcOffset + layer * faceSize),
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource  = { VK_IMAGE_ASPECT_COLOR_BIT, mip, layer, 1 },
            .imageOffset       = { 0, 0, 0 },
            .imageExtent       = { mipWidth, mipHeight, 1 },
        };
    }

    vkCmdCopyBufferToImage(
        SenVk::LookupCommandList(list).Cmd,
        SenVk::LookupBuffer(src).Buffer, entry.Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t(regions.size()), regions.data()
    );
}

auto SenCmd::Transition(SenCommandList list, const SenTransition* transitions, uint32_t count) -> void {
    if (count == 0) { return; }

    std::vector<VkImageMemoryBarrier2> barriers;
    barriers.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        const auto& transition = transitions[i];
        const auto& subresource = transition.Subresource;
        const auto& entry = SenVk::LookupTexture(transition.Texture);

        const uint32_t mipCount = subresource.MipCount == SenAllMips
            ? entry.MipLevels - subresource.BaseMip
            : subresource.MipCount;
        const uint32_t layerCount = subresource.LayerCount == SenAllLayers
            ? entry.LayerCount - subresource.BaseLayer
            : subresource.LayerCount;

        be_assert(
            subresource.BaseMip + mipCount <= entry.MipLevels,
            "SenCmd::Transition: mip range out of bounds", subresource.BaseMip, mipCount, entry.MipLevels
        );
        be_assert(
            subresource.BaseLayer + layerCount <= entry.LayerCount,
            "SenCmd::Transition: layer range out of bounds", subresource.BaseLayer, layerCount, entry.LayerCount
        );

        const VkImageAspectFlags aspect = 
            (entry.Format == VK_FORMAT_D32_SFLOAT) 
            ? VK_IMAGE_ASPECT_DEPTH_BIT 
            : VK_IMAGE_ASPECT_COLOR_BIT;
        
        const VkImageSubresourceRange range {
            .aspectMask = aspect,
            .baseMipLevel = subresource.BaseMip,
            .levelCount = mipCount,
            .baseArrayLayer = subresource.BaseLayer,
            .layerCount = layerCount,
        };
        barriers.push_back(SenVk::MakeImageBarrier(
            entry.Image, range, SenVk::ToImageLayout(transition.From), SenVk::ToImageLayout(transition.To)
        ));
    }

    const VkDependencyInfo dependency {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = uint32_t(barriers.size()),
        .pImageMemoryBarriers = barriers.data(),
    };
    vkCmdPipelineBarrier2(SenVk::LookupCommandList(list).Cmd, &dependency);
}

auto SenCmd::Begin(SenCommandList list) -> void {
    auto& entry = SenVk::LookupCommandList(list);

    entry.BoundPipelineLayout = VK_NULL_HANDLE;
    entry.BoundPipeline       = {};
    entry.PipelineDirty       = false;

    entry.CullDirty = true;
    entry.FrontFaceDirty = true;
    entry.DepthDirty = true;
    entry.DepthBiasDirty = true;

    vkResetCommandBuffer(entry.Cmd, 0);
    const VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(entry.Cmd, &beginInfo);
}

auto SenCmd::End(SenCommandList list) -> void {
    vkEndCommandBuffer(SenVk::LookupCommandList(list).Cmd);
}

auto SenCmd::PushMarker(SenCommandList list, const char* label) -> void {
}

auto SenCmd::PopMarker(SenCommandList list) -> void {
}


// ─── pipeline + resources ─────────────────────────────────────────────────────

auto SenCmd::SetPipeline(SenCommandList list, SenPipeline pipeline) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    if (pipeline == entry.BoundPipeline) {
        return;
    }

    const auto& pipelineEntry = SenVk::LookupPipeline(pipeline);
    entry.BoundPipelineLayout = pipelineEntry.Layout;
    entry.BoundPipeline = pipeline;
    entry.PipelineDirty = true;
}

auto SenCmd::SetCull(SenCommandList list, SenCull mode) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    entry.CullDirty |= (mode != entry.Cull);
    entry.Cull = mode;
}

auto SenCmd::SetFrontFace(SenCommandList list, SenFrontFace frontFace) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    entry.FrontFaceDirty |= (frontFace != entry.FrontFace);
    entry.FrontFace = frontFace;
}

auto SenCmd::SetDepth(SenCommandList list, const SenDepthState& depth) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    entry.DepthDirty |=
        depth.Test != entry.Depth.Test ||
        depth.Write != entry.Depth.Write ||
        depth.Compare != entry.Depth.Compare;
    entry.Depth = depth;
}

auto SenCmd::SetDepthBias(SenCommandList list, float constant, float slopeScaled) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    entry.DepthBiasDirty |= (constant != entry.DepthBiasConstant || slopeScaled != entry.DepthBiasSlope);
    entry.DepthBiasConstant = constant;
    entry.DepthBiasSlope = slopeScaled;
}

auto SenCmd::SetRoot(SenCommandList list, const void* data, uint32_t size) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    be_assert(entry.BoundPipelineLayout != VK_NULL_HANDLE, "SetRoot: no pipeline bound");
    be_assert(size <= SenMaxRootSize, "SetRoot: root struct exceeds {} bytes", SenMaxRootSize);
    vkCmdPushConstants(entry.Cmd, entry.BoundPipelineLayout, VK_SHADER_STAGE_ALL, 0, size, data);
}

auto SenVk::FlushPipeline(SenVulkanCommandListEntry& entry) -> void {
    if (!entry.PipelineDirty) {
        return;
    }
    const auto& pipelineEntry = SenVk::LookupPipeline(entry.BoundPipeline);
    vkCmdBindPipeline(entry.Cmd, pipelineEntry.BindPoint, pipelineEntry.Pipeline);
    VkDescriptorSet heap = SenVk::_bindlessSet;
    vkCmdBindDescriptorSets(entry.Cmd, pipelineEntry.BindPoint, pipelineEntry.Layout, 0, 1, &heap, 0, nullptr);
    entry.PipelineDirty = false;
}

auto SenVk::FlushDrawState(SenVulkanCommandListEntry& entry) -> void {
    if (entry.CullDirty) {
        vkCmdSetCullMode(entry.Cmd, SenVk::ToCull(entry.Cull));
        entry.CullDirty = false;
    }

    if (entry.FrontFaceDirty) {
        vkCmdSetFrontFace(entry.Cmd, SenVk::ToFrontFace(entry.FrontFace));
        entry.FrontFaceDirty = false;
    }

    if (entry.DepthDirty) {
        vkCmdSetDepthTestEnable(entry.Cmd, entry.Depth.Test);
        vkCmdSetDepthWriteEnable(entry.Cmd, entry.Depth.Write);
        vkCmdSetDepthCompareOp(entry.Cmd, SenVk::ToCompareOp(entry.Depth.Compare));
        entry.DepthDirty = false;
    }

    if (entry.DepthBiasDirty) {
        const bool enable = entry.DepthBiasConstant != 0.f || entry.DepthBiasSlope != 0.f;
        vkCmdSetDepthBiasEnable(entry.Cmd, enable);
        vkCmdSetDepthBias(entry.Cmd, entry.DepthBiasConstant, 0.f, entry.DepthBiasSlope);
        entry.DepthBiasDirty = false;
    }
}

auto SenCmd::SetVertexBuffer(SenCommandList list, SenBuffer buffer) -> void {
    auto& bufferEntry = SenVk::LookupBuffer(buffer);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(SenVk::LookupCommandList(list).Cmd, 0, 1, &bufferEntry.Buffer, &offset);
}

auto SenCmd::SetIndexBuffer(SenCommandList list, SenBuffer buffer) -> void {
    auto& bufferEntry = SenVk::LookupBuffer(buffer);
    vkCmdBindIndexBuffer(SenVk::LookupCommandList(list).Cmd, bufferEntry.Buffer, 0, VK_INDEX_TYPE_UINT32);
}


// ─── draw ─────────────────────────────────────────────────────────────────────

auto SenCmd::Draw(SenCommandList list, uint32_t vertexCount, uint32_t firstVertex) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    FlushPipeline(entry);
    FlushDrawState(entry);
    vkCmdDraw(entry.Cmd, vertexCount, 1, firstVertex, 0);
}

auto SenCmd::DrawIndexed(SenCommandList list, uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    FlushPipeline(entry);
    FlushDrawState(entry);
    vkCmdDrawIndexed(entry.Cmd, indexCount, 1, firstIndex, baseVertex, 0);
}

auto SenCmd::Dispatch(SenCommandList list, uint32_t x, uint32_t y, uint32_t z) -> void {
    auto& entry = SenVk::LookupCommandList(list);
    FlushPipeline(entry);
    vkCmdDispatch(entry.Cmd, x, y, z);
}
