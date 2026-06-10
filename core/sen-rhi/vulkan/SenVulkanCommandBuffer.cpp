#include "SenVulkanCommandBuffer.h"
#include "SenVulkanBackend.h"
#include "SenVulkanConvert.h"

#include <umbrellas/include-libassert.h>
#include <umbrellas/include-glm.h>

SenVulkanCommandBuffer::SenVulkanCommandBuffer(VkCommandBuffer cmd)
    : _cmd(cmd)
{}


// ─── render pass ──────────────────────────────────────────────────────────────

auto SenVulkanCommandBuffer::BeginPass(const SenPassDesc& desc) -> void {
    // Attachments are expected to already be in the correct layout — callers transition
    // them (e.g. via BePass / TransitionTextures). BeginPass only opens the render pass.

    // Color attachments
    std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
    colorAttachments.reserve(desc.ColorAttachments.size());

    for (const auto& attachment : desc.ColorAttachments) {
        auto& texEntry = SenVulkanBackend::LookupTexture(attachment.Texture);

        VkImageView view = VK_NULL_HANDLE;
        if (attachment.CubemapFace >= 0) {
            be_assert(attachment.CubemapFace < 6, "BeginPass: invalid cubemap face");
            view = texEntry.CubemapMipRTVs[attachment.CubemapFace][attachment.MipLevel];
        } else {
            view = texEntry.MipRTVs[attachment.MipLevel];
        }

        VkClearValue clearValue {};
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
        auto& texEntry = SenVulkanBackend::LookupTexture(depthAttach.Texture);

        VkImageView view = VK_NULL_HANDLE;
        if (depthAttach.CubemapFace >= 0) {
            be_assert(depthAttach.CubemapFace < 6, "BeginPass: invalid cubemap face for depth");
            view = texEntry.CubemapDSVs[depthAttach.CubemapFace];
        } else {
            view = texEntry.DSV;
        }

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

    vkCmdBeginRendering(_cmd, &renderingInfo);

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
    vkCmdSetViewport(_cmd, 0, 1, &vp);

    VkRect2D scissor {
        .offset = { int32_t(desc.Viewport.X), int32_t(desc.Viewport.Y) },
        .extent = { uint32_t(desc.Viewport.Width), uint32_t(desc.Viewport.Height) },
    };
    vkCmdSetScissor(_cmd, 0, 1, &scissor);
}

auto SenVulkanCommandBuffer::EndPass() -> void {
    vkCmdEndRendering(_cmd);
}

auto SenVulkanCommandBuffer::TransitionTextures(const std::vector<std::pair<SenTexture, SenResourceState>>& transitions) -> void {
    std::vector<VkImageMemoryBarrier2> barriers;
    barriers.reserve(transitions.size());

    for (const auto& [texture, state] : transitions) {
        auto& entry = SenVulkanBackend::LookupTexture(texture);
        const VkImageLayout oldLayout = entry.CurrentLayout;
        const VkImageLayout newLayout = Sen::Vulkan::ToImageLayout(state);
        if (oldLayout == newLayout) { continue; }  // already in the target state — skip

        const VkImageAspectFlags aspect = (entry.Format == VK_FORMAT_D32_SFLOAT)
            ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        VkPipelineStageFlags2 srcStage, dstStage;
        VkAccessFlags2 srcAccess, dstAccess;
        Sen::Vulkan::ScopeForLayout(oldLayout, srcStage, srcAccess);
        Sen::Vulkan::ScopeForLayout(newLayout, dstStage, dstAccess);

        barriers.push_back(VkImageMemoryBarrier2 {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = srcStage,
            .srcAccessMask       = srcAccess,
            .dstStageMask        = dstStage,
            .dstAccessMask       = dstAccess,
            .oldLayout           = oldLayout,
            .newLayout           = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = entry.Image,
            .subresourceRange    = { aspect, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS },
        });
        entry.CurrentLayout = newLayout;
    }

    if (barriers.empty()) { return; }

    const VkDependencyInfo dependency {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = uint32_t(barriers.size()),
        .pImageMemoryBarriers    = barriers.data(),
    };
    vkCmdPipelineBarrier2(_cmd, &dependency);
}

auto SenVulkanCommandBuffer::ResetPerFrameState() -> void {
    _boundPipelineLayout = VK_NULL_HANDLE;
    _pendingBindGroupDirty = {};
}


// ─── pipeline + resources ─────────────────────────────────────────────────────

auto SenVulkanCommandBuffer::SetPipeline(SenPipeline pipeline) -> void {
    auto& entry = SenVulkanBackend::LookupPipeline(pipeline);
    _boundPipelineLayout = entry.Layout;
    _boundPipeline = pipeline;
    vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, entry.Pipeline);
    FlushPendingBindGroups();
}

auto SenVulkanCommandBuffer::SetBindGroup(SenBindGroup group, uint8_t index) -> void {
    be_assert(group.IsValid(), "SetBindGroup: invalid SenBindGroup handle (index={})", index);
    auto& groupEntry = SenVulkanBackend::LookupBindGroup(group);

    if (_boundPipelineLayout == VK_NULL_HANDLE) {
        _pendingBindGroups[index]     = group;
        _pendingBindGroupDirty[index] = true;
        return;
    }

    vkCmdBindDescriptorSets(
        _cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _boundPipelineLayout,
        index,
        1, &groupEntry.Set,
        0, nullptr
    );
}

auto SenVulkanCommandBuffer::FlushPendingBindGroups() -> void {
    for (uint8_t i = 0; i < MaxBindGroups; ++i) {
        if (!_pendingBindGroupDirty[i]) {
            continue;
        }
        auto& groupEntry = SenVulkanBackend::LookupBindGroup(_pendingBindGroups[i]);
        vkCmdBindDescriptorSets(
            _cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _boundPipelineLayout,
            i,
            1, &groupEntry.Set,
            0, nullptr
        );
        _pendingBindGroupDirty[i] = false;
    }
}

auto SenVulkanCommandBuffer::SetVertexBuffer(SenBuffer buffer) -> void {
    auto& entry = SenVulkanBackend::LookupBuffer(buffer);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_cmd, 0, 1, &entry.Buffer, &offset);
}

auto SenVulkanCommandBuffer::SetIndexBuffer(SenBuffer buffer) -> void {
    auto& entry = SenVulkanBackend::LookupBuffer(buffer);
    vkCmdBindIndexBuffer(_cmd, entry.Buffer, 0, VK_INDEX_TYPE_UINT32);
}


// ─── draw ─────────────────────────────────────────────────────────────────────

auto SenVulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex) -> void {
    vkCmdDraw(_cmd, vertexCount, 1, firstVertex, 0);
}

auto SenVulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void {
    vkCmdDrawIndexed(_cmd, indexCount, 1, firstIndex, baseVertex, 0);
}
