#include "SenVulkanBackend.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::CreatePipeline(const SenPipelineDesc& desc) -> SenPipeline {
    const SenPipeline handle { _nextPipelineId++ };
    _pipelines[handle.ID] = MakePipelineEntry(desc);
    return handle;
}

auto SenVulkanBackend::MakePipelineEntry(const SenPipelineDesc& desc) -> SenVulkanPipelineEntry {
    SenVulkanPipelineEntry entry {};
    entry.Desc = desc;

    // ── pipeline layout (from bind group layout descriptors) ──────────────────────────────
    std::vector<VkDescriptorSetLayout> setLayouts;
    setLayouts.reserve(desc.BindGroupLayouts.size());
    for (const auto& bgl : desc.BindGroupLayouts) {
        setLayouts.push_back(CreateDescriptorSetLayoutFromDesc(bgl));
    }

    VkPipelineLayoutCreateInfo layoutInfo {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = uint32_t(setLayouts.size()),
        .pSetLayouts    = setLayouts.data(),
    };
    VkResult result = vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &entry.Layout);
    be_assert(result == VK_SUCCESS, "Failed to create pipeline layout!");

    for (auto layout : setLayouts) {
        vkDestroyDescriptorSetLayout(_device, layout, nullptr);
    }

    // ── compute pipeline ───────────────────────────────────────────────────────
    if (desc.ComputeShader.IsValid()) {
        be_assert(!desc.VertexShader.IsValid(), "CreatePipeline: ComputeShader and VertexShader are mutually exclusive");

        VkPipelineShaderStageCreateInfo computeStage {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = LookupShader(desc.ComputeShader).Module,
            .pName  = "main",
        };
        VkComputePipelineCreateInfo computeInfo {
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = computeStage,
            .layout = entry.Layout,
        };
        result = vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &entry.Pipeline);
        be_assert(result == VK_SUCCESS, "Failed to create compute pipeline!");

        entry.BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
        return entry;
    }

    // ── shader stages ──────────────────────────────────────────────────────────
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    auto addStage = [&](SenShader shader, VkShaderStageFlagBits stageBit) {
        if (!shader.IsValid()) { return; }
        stages.push_back(VkPipelineShaderStageCreateInfo {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = stageBit,
            .module = LookupShader(shader).Module,
            .pName  = "main",
        });
    };
    addStage(desc.VertexShader, VK_SHADER_STAGE_VERTEX_BIT);
    addStage(desc.HullShader,   VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    addStage(desc.DomainShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    addStage(desc.PixelShader,  VK_SHADER_STAGE_FRAGMENT_BIT);

    // ── vertex input ───────────────────────────────────────────────────────────
    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(desc.VertexLayout.size());
    for (const auto& elem : desc.VertexLayout) {
        attributes.push_back(VkVertexInputAttributeDescription {
            .location = elem.Location,
            .binding  = 0,
            .format   = Sen::Vulkan::ToFormat(elem.Format),
            .offset   = elem.Offset,
        });
    }

    uint32_t vertexStride = desc.VertexStride;

    VkVertexInputBindingDescription binding {
        .binding   = 0,
        .stride    = vertexStride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkPipelineVertexInputStateCreateInfo vertexInput {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = desc.VertexLayout.empty() ? 0u : 1u,
        .pVertexBindingDescriptions      = desc.VertexLayout.empty() ? nullptr : &binding,
        .vertexAttributeDescriptionCount = uint32_t(attributes.size()),
        .pVertexAttributeDescriptions    = attributes.data(),
    };

    // ── input assembly ─────────────────────────────────────────────────────────
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = Sen::Vulkan::ToTopology(desc.Topology),
        .primitiveRestartEnable = VK_FALSE,
    };

    // ── tessellation ───────────────────────────────────────────────────────────
    VkPipelineTessellationStateCreateInfo tessellation {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = desc.Topology == SenTopology::PatchList3 ? 3u : 1u,
    };

    // ── viewport (dynamic) ─────────────────────────────────────────────────────
    VkPipelineViewportStateCreateInfo viewportState {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount  = 1,
    };

    // ── rasterizer ─────────────────────────────────────────────────────────────
    VkPipelineRasterizationStateCreateInfo rasterizer {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = !desc.RasterizerState.DepthClipEnable,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = Sen::Vulkan::ToFillMode(desc.RasterizerState.FillMode),
        .cullMode                = Sen::Vulkan::ToCullMode(desc.RasterizerState.CullMode),
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = desc.RasterizerState.DepthBias != 0.f || desc.RasterizerState.SlopeScaledDepthBias != 0.f,
        .depthBiasConstantFactor = desc.RasterizerState.DepthBias,
        .depthBiasSlopeFactor    = desc.RasterizerState.SlopeScaledDepthBias,
        .lineWidth               = 1.f,
    };

    // ── multisample ────────────────────────────────────────────────────────────
    VkPipelineMultisampleStateCreateInfo multisample {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    // ── depth stencil ──────────────────────────────────────────────────────────
    VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = desc.DepthStencilState.DepthEnable,
        .depthWriteEnable = desc.DepthStencilState.DepthWriteEnable,
        .depthCompareOp   = Sen::Vulkan::ToCompareOp(desc.DepthStencilState.DepthFunc),
    };

    // ── blend ──────────────────────────────────────────────────────────────────
    uint32_t rtCount = uint32_t(desc.RenderTargetFormats.size());
    VkPipelineColorBlendAttachmentState blendAttachment {
        .blendEnable         = desc.BlendState.Enable,
        .srcColorBlendFactor = Sen::Vulkan::ToBlendFactor(desc.BlendState.SrcBlend),
        .dstColorBlendFactor = Sen::Vulkan::ToBlendFactor(desc.BlendState.DstBlend),
        .colorBlendOp        = Sen::Vulkan::ToBlendOp(desc.BlendState.BlendOp),
        .srcAlphaBlendFactor = Sen::Vulkan::ToBlendFactor(desc.BlendState.SrcBlendAlpha),
        .dstAlphaBlendFactor = Sen::Vulkan::ToBlendFactor(desc.BlendState.DstBlendAlpha),
        .alphaBlendOp        = Sen::Vulkan::ToBlendOp(desc.BlendState.BlendOpAlpha),
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(rtCount, blendAttachment);

    VkPipelineColorBlendStateCreateInfo blendState {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = uint32_t(blendAttachments.size()),
        .pAttachments    = blendAttachments.data(),
    };

    // ── dynamic state ──────────────────────────────────────────────────────────
    std::array<VkDynamicState, 2> dynamicStates { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = uint32_t(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data(),
    };

    // ── dynamic rendering ──────────────────────────────────────────────────────
    std::vector<VkFormat> colorFormats;
    colorFormats.reserve(desc.RenderTargetFormats.size());
    for (const auto& fmt : desc.RenderTargetFormats) {
        colorFormats.push_back(Sen::Vulkan::ToFormat(fmt));
    }
    VkFormat depthFormat = Sen::Vulkan::ToFormat(desc.DepthStencilFormat);

    VkPipelineRenderingCreateInfoKHR renderingInfo {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount    = uint32_t(colorFormats.size()),
        .pColorAttachmentFormats = colorFormats.data(),
        .depthAttachmentFormat   = depthFormat,
    };

    // ── create pipeline ────────────────────────────────────────────────────────
    VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &renderingInfo,
        .stageCount          = uint32_t(stages.size()),
        .pStages             = stages.data(),
        .pVertexInputState   = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState  = &tessellation,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisample,
        .pDepthStencilState  = &depthStencil,
        .pColorBlendState    = &blendState,
        .pDynamicState       = &dynamicState,
        .layout              = entry.Layout,
    };

    result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &entry.Pipeline);
    be_assert(result == VK_SUCCESS, "Failed to create graphics pipeline!");

    return entry;
}

auto SenVulkanBackend::DestroyPipeline(SenPipeline handle) -> void {
    auto it = _pipelines.find(handle.ID);
    if (it != _pipelines.end()) {
        vkDestroyPipeline(_device, it->second.Pipeline, nullptr);
        vkDestroyPipelineLayout(_device, it->second.Layout, nullptr);
        _pipelines.erase(it);
    }
}

auto SenVulkanBackend::LookupPipeline(SenPipeline handle) -> SenVulkanPipelineEntry& {
    return _pipelines.at(handle.ID);
}
