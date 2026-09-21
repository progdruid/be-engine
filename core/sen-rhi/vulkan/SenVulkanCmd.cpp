#include "SenVulkanCmd.h"
#include "SenVulkanBackend.h"
#include "SenVulkanConvert.h"

#include <algorithm>
#include <umbrellas/include-libassert.h>
#include <umbrellas/include-glm.h>


// ─── render pass ──────────────────────────────────────────────────────────────

auto SenVulkanCmd::BeginPass(SenCommandList list, const SenPassDesc& desc) -> void {
    // Attachments are expected to already be in the correct layout — callers transition
    // them (e.g. via BePass / TransitionTextures). BeginPass only opens the render pass.
    const VkCommandBuffer cmd = SenVulkanBackend::LookupCommandList(list).Cmd;

    // Color attachments
    std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
    colorAttachments.reserve(desc.ColorAttachments.size());

    for (const auto& attachment : desc.ColorAttachments) {
        const VkImageView view = SenVulkanBackend::LookupView(attachment.View).View;

        VkClearValue clearValue;
        clearValue.color = { attachment.ClearColor.r, attachment.ClearColor.g, attachment.ClearColor.b, attachment.ClearColor.a };

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
        const VkImageView view = SenVulkanBackend::LookupView(depthAttach.View).View;

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

auto SenVulkanCmd::EndPass(SenCommandList list) -> void {
    vkCmdEndRendering(SenVulkanBackend::LookupCommandList(list).Cmd);
}

auto SenVulkanCmd::CopyBuffer(SenCommandList list, SenBuffer src, uint32_t srcOffset, uint32_t size, SenBuffer dst, uint32_t dstOffset) -> void {
    const VkBufferCopy region {
        .srcOffset = srcOffset,
        .dstOffset = dstOffset,
        .size      = size,
    };
    vkCmdCopyBuffer(
        SenVulkanBackend::LookupCommandList(list).Cmd,
        SenVulkanBackend::LookupBuffer(src).Buffer,
        SenVulkanBackend::LookupBuffer(dst).Buffer,
        1, &region
    );
}

auto SenVulkanCmd::CopyBufferToTexture(SenCommandList list, SenBuffer src, uint32_t srcOffset, SenTexture dst, uint32_t mip) -> void {
    auto& entry = SenVulkanBackend::LookupTexture(dst);
    be_assert(mip < entry.MipLevels, "CopyBufferToTexture: mip out of range");

    const uint32_t mipWidth  = std::max(1u, entry.Width >> mip);
    const uint32_t mipHeight = std::max(1u, entry.Height >> mip);
    const uint32_t faceSize  = mipWidth * mipHeight * SenGetFormatBytes(Sen::Vulkan::FromVkFormat(entry.Format));

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
        SenVulkanBackend::LookupCommandList(list).Cmd,
        SenVulkanBackend::LookupBuffer(src).Buffer, entry.Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t(regions.size()), regions.data()
    );
}

auto SenVulkanCmd::TransitionTextures(SenCommandList list, const std::vector<SenTextureTransition>& transitions) -> void {
    std::vector<VkImageMemoryBarrier2> barriers;
    barriers.reserve(transitions.size());

    for (const auto& transition : transitions) {
        auto& entry = SenVulkanBackend::LookupTexture(transition.Texture);
        const VkImageLayout newLayout = Sen::Vulkan::ToImageLayout(transition.State);

        const VkImageAspectFlags aspect = (entry.Format == VK_FORMAT_D32_SFLOAT)
            ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        const uint32_t baseMip  = transition.BaseMip;
        const uint32_t mipCount = transition.MipCount == SenAllMips ? entry.MipLevels - baseMip : transition.MipCount;

        // One barrier per mip whose layout actually changes — mips may sit in different layouts
        // (e.g. bloom samples mip i while rendering into mip i+1 of the same image).
        for (uint32_t mip = baseMip; mip < baseMip + mipCount; ++mip) {
            const VkImageLayout oldLayout = entry.MipLayouts[mip];
            if (oldLayout == newLayout) { continue; }

            const VkImageSubresourceRange range { aspect, mip, 1, 0, VK_REMAINING_ARRAY_LAYERS };
            barriers.push_back(SenVulkanBackend::MakeImageBarrier(entry.Image, range, oldLayout, newLayout));
            entry.MipLayouts[mip] = newLayout;
        }
    }

    if (barriers.empty()) { return; }

    const VkDependencyInfo dependency {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = uint32_t(barriers.size()),
        .pImageMemoryBarriers    = barriers.data(),
    };
    vkCmdPipelineBarrier2(SenVulkanBackend::LookupCommandList(list).Cmd, &dependency);
}

auto SenVulkanCmd::Begin(SenCommandList list) -> void {
    auto& entry = SenVulkanBackend::LookupCommandList(list);

    entry.BoundPipelineLayout = VK_NULL_HANDLE;
    entry.BoundPipeline       = {};
    entry.PipelineDirty       = false;

    vkResetCommandBuffer(entry.Cmd, 0);
    const VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(entry.Cmd, &beginInfo);
}

auto SenVulkanCmd::End(SenCommandList list) -> void {
    vkEndCommandBuffer(SenVulkanBackend::LookupCommandList(list).Cmd);
}


// ─── pipeline + resources ─────────────────────────────────────────────────────

auto SenVulkanCmd::SetPipeline(SenCommandList list, SenPipeline pipeline) -> void {
    auto& entry = SenVulkanBackend::LookupCommandList(list);
    if (pipeline == entry.BoundPipeline) {
        return;
    }

    const auto& pipelineEntry = SenVulkanBackend::LookupPipeline(pipeline);
    entry.BoundPipelineLayout = pipelineEntry.Layout;
    entry.BoundPipeline = pipeline;
    entry.PipelineDirty = true;
}

auto SenVulkanCmd::PushRoot(SenCommandList list, const void* data, uint32_t size) -> void {
    auto& entry = SenVulkanBackend::LookupCommandList(list);
    be_assert(entry.BoundPipelineLayout != VK_NULL_HANDLE, "PushRoot: no pipeline bound");
    be_assert(size <= SenMaxRootConstantSize, "PushRoot: root struct exceeds {} bytes", SenMaxRootConstantSize);
    vkCmdPushConstants(entry.Cmd, entry.BoundPipelineLayout, VK_SHADER_STAGE_ALL, 0, size, data);
}

auto SenVulkanCmd::FlushState(SenVulkanCommandListEntry& entry) -> void {
    if (!entry.PipelineDirty) {
        return;
    }
    const auto& pipelineEntry = SenVulkanBackend::LookupPipeline(entry.BoundPipeline);
    vkCmdBindPipeline(entry.Cmd, pipelineEntry.BindPoint, pipelineEntry.Pipeline);
    VkDescriptorSet heap = SenVulkanBackend::GetBindlessSet();
    vkCmdBindDescriptorSets(entry.Cmd, pipelineEntry.BindPoint, pipelineEntry.Layout, 0, 1, &heap, 0, nullptr);
    entry.PipelineDirty = false;
}

auto SenVulkanCmd::SetVertexBuffer(SenCommandList list, SenBuffer buffer) -> void {
    auto& bufferEntry = SenVulkanBackend::LookupBuffer(buffer);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(SenVulkanBackend::LookupCommandList(list).Cmd, 0, 1, &bufferEntry.Buffer, &offset);
}

auto SenVulkanCmd::SetIndexBuffer(SenCommandList list, SenBuffer buffer) -> void {
    auto& bufferEntry = SenVulkanBackend::LookupBuffer(buffer);
    vkCmdBindIndexBuffer(SenVulkanBackend::LookupCommandList(list).Cmd, bufferEntry.Buffer, 0, VK_INDEX_TYPE_UINT32);
}

auto SenVulkanCmd::GetNativeHandle(SenCommandList list) -> VkCommandBuffer {
    return SenVulkanBackend::LookupCommandList(list).Cmd;
}


// ─── draw ─────────────────────────────────────────────────────────────────────

auto SenVulkanCmd::Draw(SenCommandList list, uint32_t vertexCount, uint32_t firstVertex) -> void {
    auto& entry = SenVulkanBackend::LookupCommandList(list);
    FlushState(entry);
    vkCmdDraw(entry.Cmd, vertexCount, 1, firstVertex, 0);
}

auto SenVulkanCmd::DrawIndexed(SenCommandList list, uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void {
    auto& entry = SenVulkanBackend::LookupCommandList(list);
    FlushState(entry);
    vkCmdDrawIndexed(entry.Cmd, indexCount, 1, firstIndex, baseVertex, 0);
}

auto SenVulkanCmd::Dispatch(SenCommandList list, uint32_t x, uint32_t y, uint32_t z) -> void {
    auto& entry = SenVulkanBackend::LookupCommandList(list);
    FlushState(entry);
    vkCmdDispatch(entry.Cmd, x, y, z);
}
