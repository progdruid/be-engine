#include "SenVulkanState.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

using namespace SenVk;

auto Sen::CreatePipeline(const SenPipelineDesc& desc) -> SenPipeline {
    auto entry = SenVulkanPipelineEntry();

    auto createModule = [&](const SenShaderCode& code) -> VkShaderModule {
        VkShaderModuleCreateInfo moduleInfo {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.Count * sizeof(uint32_t),
            .pCode    = code.Code,
        };
        VkShaderModule module = VK_NULL_HANDLE;
        VkResult moduleResult = vkCreateShaderModule(_device, &moduleInfo, nullptr, &module);
        be_assert(moduleResult == VK_SUCCESS, "Failed to create shader module!");
        return module;
    };

    // ── pipeline layout ───────────────────────────────────────────────────────
    // Set 0 is the persistent heap, all per-draw data arrives as root push constants.
    const VkPushConstantRange rootRange {
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset     = 0,
        .size       = SenMaxRootSize,
    };
    VkPipelineLayoutCreateInfo layoutInfo {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1,
        .pSetLayouts            = &_bindlessLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &rootRange,
    };
    VkResult result = vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &entry.Layout);
    be_assert(result == VK_SUCCESS, "Failed to create pipeline layout!");

    // ── compute pipeline ───────────────────────────────────────────────────────
    if (desc.ComputeShader.IsValid()) {
        be_assert(!desc.VertexShader.IsValid(), "CreatePipeline: ComputeShader and VertexShader are mutually exclusive");

        const VkShaderModule computeModule = createModule(desc.ComputeShader);
        VkPipelineShaderStageCreateInfo computeStage {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = computeModule,
            .pName  = "main",
        };
        VkComputePipelineCreateInfo computeInfo {
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = computeStage,
            .layout = entry.Layout,
        };
        result = vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &entry.Pipeline);
        be_assert(result == VK_SUCCESS, "Failed to create compute pipeline!");
        vkDestroyShaderModule(_device, computeModule, nullptr);

        entry.BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
        return _pipelines.Create(std::move(entry));
    }

    // ── shader stages ──────────────────────────────────────────────────────────
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkShaderModule> modules;

    auto addStage = [&](const SenShaderCode& code, VkShaderStageFlagBits stageBit) {
        if (!code.IsValid()) { return; }
        modules.push_back(createModule(code));
        stages.push_back(VkPipelineShaderStageCreateInfo {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = stageBit,
            .module = modules.back(),
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
            .format   = SenVk::ToFormat(elem.Format),
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
        .topology               = SenVk::ToTopology(desc.Topology),
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
    // cullMode, frontFace and the depthBias fields are dynamic; the values here are ignored.
    VkPipelineRasterizationStateCreateInfo rasterizer {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = !desc.DepthClipEnable,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = SenVk::ToFill(desc.Fill),
        .lineWidth               = 1.f,
    };

    // ── multisample ────────────────────────────────────────────────────────────
    VkPipelineMultisampleStateCreateInfo multisample {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    // ── depth stencil ──────────────────────────────────────────────────────────
    // Fully dynamic; the struct must still be present when there is a depth attachment.
    VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    // ── blend ──────────────────────────────────────────────────────────────────
    uint32_t rtCount = uint32_t(desc.RenderTargetFormats.size());
    VkPipelineColorBlendAttachmentState blendAttachment {
        .blendEnable         = desc.BlendState.Enable,
        .srcColorBlendFactor = SenVk::ToBlendFactor(desc.BlendState.SrcBlend),
        .dstColorBlendFactor = SenVk::ToBlendFactor(desc.BlendState.DstBlend),
        .colorBlendOp        = SenVk::ToBlendOp(desc.BlendState.BlendOp),
        .srcAlphaBlendFactor = SenVk::ToBlendFactor(desc.BlendState.SrcBlendAlpha),
        .dstAlphaBlendFactor = SenVk::ToBlendFactor(desc.BlendState.DstBlendAlpha),
        .alphaBlendOp        = SenVk::ToBlendOp(desc.BlendState.BlendOpAlpha),
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
    std::array<VkDynamicState, 9> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_CULL_MODE,
        VK_DYNAMIC_STATE_FRONT_FACE,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
        VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
    };
    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = uint32_t(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data(),
    };

    // ── dynamic rendering ──────────────────────────────────────────────────────
    std::vector<VkFormat> colorFormats;
    colorFormats.reserve(desc.RenderTargetFormats.size());
    for (const auto& fmt : desc.RenderTargetFormats) {
        colorFormats.push_back(SenVk::ToFormat(fmt));
    }
    VkFormat depthFormat = SenVk::ToFormat(desc.DepthFormat);

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

    for (const auto module : modules) {
        vkDestroyShaderModule(_device, module, nullptr);
    }

    return _pipelines.Create(std::move(entry));
}

auto Sen::DestroyPipeline(SenPipeline handle) -> void {
    if (!_pipelines.Contains(handle)) {
        return;
    }

    auto& entry = _pipelines.Get(handle);
    vkDestroyPipeline(_device, entry.Pipeline, nullptr);
    vkDestroyPipelineLayout(_device, entry.Layout, nullptr);
    _pipelines.Destroy(handle);
}

auto SenVk::LookupPipeline(SenPipeline handle) -> SenVulkanPipelineEntry& {
    return _pipelines.Get(handle);
}
