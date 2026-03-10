#include "SenVulkanCommandBuffer.h"
#include "SenVulkanBackend.h"

#include <umbrellas/include-libassert.h>
#include <umbrellas/include-glm.h>

SenVulkanCommandBuffer::SenVulkanCommandBuffer(VkCommandBuffer cmd)
    : _cmd(cmd)
{}


// ─── render pass ──────────────────────────────────────────────────────────────

auto SenVulkanCommandBuffer::BeginPass(const SenPassDesc& desc) -> void {
    // Auto-transition attachments to their expected layouts
    for (const auto& attachment : desc.ColorAttachments) {
        auto& texEntry = SenVulkanBackend::LookupTexture(attachment.Texture);
        if (texEntry.CurrentLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            SenVulkanBackend::TransitionImageLayout(
                _cmd, texEntry.Image, VK_IMAGE_ASPECT_COLOR_BIT,
                texEntry.CurrentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_REMAINING_MIP_LEVELS, VK_REMAINING_ARRAY_LAYERS
            );
            texEntry.CurrentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
    }
    if (desc.DepthAttachment.has_value()) {
        auto& texEntry = SenVulkanBackend::LookupTexture(desc.DepthAttachment->Texture);
        if (texEntry.CurrentLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            SenVulkanBackend::TransitionImageLayout(
                _cmd, texEntry.Image, VK_IMAGE_ASPECT_DEPTH_BIT,
                texEntry.CurrentLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_REMAINING_MIP_LEVELS, VK_REMAINING_ARRAY_LAYERS
            );
            texEntry.CurrentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }

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

    SenVulkanBackend::_vkCmdBeginRenderingKHR(_cmd, &renderingInfo);

    VkViewport vp {
        .x        = desc.Viewport.X,
        .y        = desc.Viewport.Y,
        .width    = desc.Viewport.Width,
        .height   = desc.Viewport.Height,
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
    SenVulkanBackend::_vkCmdEndRenderingKHR(_cmd);
}


// ─── pipeline + resources ─────────────────────────────────────────────────────

auto SenVulkanCommandBuffer::SetPipeline(SenPipeline pipeline) -> void {
    auto& entry = SenVulkanBackend::LookupPipeline(pipeline);
    _boundPipelineLayout = entry.Layout;
    vkCmdBindPipeline(_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, entry.Pipeline);
}

auto SenVulkanCommandBuffer::SetBindGroup(SenBindGroup group, uint8_t index) -> void {
    be_assert(_boundPipelineLayout != VK_NULL_HANDLE, "SetBindGroup called before SetPipeline");
    auto& groupEntry = SenVulkanBackend::LookupBindGroup(group);

    // Auto rt→sample: transition any image in this group that's still in an attachment layout
    for (const auto& texHandle : groupEntry.ImageTextures) {
        auto& texEntry = SenVulkanBackend::LookupTexture(texHandle);
        if (texEntry.CurrentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ||
            texEntry.CurrentLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            const VkImageAspectFlags aspect = (texEntry.CurrentLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                ? VK_IMAGE_ASPECT_DEPTH_BIT
                : VK_IMAGE_ASPECT_COLOR_BIT;
            SenVulkanBackend::TransitionImageLayout(
                _cmd, texEntry.Image, aspect,
                texEntry.CurrentLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_REMAINING_MIP_LEVELS, VK_REMAINING_ARRAY_LAYERS
            );
            texEntry.CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
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

auto SenVulkanCommandBuffer::SetVertexBuffer(SenBuffer buffer, uint32_t stride) -> void {
    auto& entry = SenVulkanBackend::LookupBuffer(buffer);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_cmd, 0, 1, &entry.Buffer, &offset);
}

auto SenVulkanCommandBuffer::SetIndexBuffer(SenBuffer buffer) -> void {
    auto& entry = SenVulkanBackend::LookupBuffer(buffer);
    vkCmdBindIndexBuffer(_cmd, entry.Buffer, 0, VK_INDEX_TYPE_UINT32);
}

auto SenVulkanCommandBuffer::ClearVertexBuffer() -> void {
    // no-op in Vulkan
}

auto SenVulkanCommandBuffer::ClearIndexBuffer() -> void {
    // no-op in Vulkan
}


// ─── draw ─────────────────────────────────────────────────────────────────────

auto SenVulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex) -> void {
    vkCmdDraw(_cmd, vertexCount, 1, firstVertex, 0);
}

auto SenVulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void {
    vkCmdDrawIndexed(_cmd, indexCount, 1, firstIndex, baseVertex, 0);
}
