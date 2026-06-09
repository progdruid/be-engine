#include "SenVulkanBackend.h"

#include <sen-rhi/SenShaderCompiler.h>
#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::CreateShader(const SenShaderSourceDesc& sourceDesc) -> SenShader {
    SlangStage slangStage = SLANG_STAGE_NONE;
    switch (sourceDesc.Stage) {
        case SenShaderStage::Vertex: slangStage = SLANG_STAGE_VERTEX; break;
        case SenShaderStage::Hull:   slangStage = SLANG_STAGE_HULL;   break;
        case SenShaderStage::Domain: slangStage = SLANG_STAGE_DOMAIN; break;
        case SenShaderStage::Pixel:  slangStage = SLANG_STAGE_PIXEL;  break;
        default: be_assert(false, "SenVulkanBackend::CreateShader: unsupported shader stage"); break;
    }

    auto compileResult = SenShaderCompiler::Compile(sourceDesc.SourcePath, sourceDesc.FunctionName, slangStage, SLANG_SPIRV);
    be_assert(compileResult, "SenVulkanBackend::CreateShader: shader compilation failed: " + compileResult.error());

    auto& blob = compileResult.value();

    VkShaderModuleCreateInfo moduleInfo {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = blob->getBufferSize(),
        .pCode    = static_cast<const uint32_t*>(blob->getBufferPointer()),
    };

    const SenShader handle { _nextShaderId++ };
    auto& entry = _shaders[handle.ID];
    entry.Stage = sourceDesc.Stage;

    VkResult result = vkCreateShaderModule(_device, &moduleInfo, nullptr, &entry.Module);
    be_assert(result == VK_SUCCESS, "Failed to create shader module!");

    return handle;
}

auto SenVulkanBackend::DestroyShader(SenShader handle) -> void {
    auto it = _shaders.find(handle.ID);
    if (it != _shaders.end()) {
        vkDestroyShaderModule(_device, it->second.Module, nullptr);
        _shaders.erase(it);
    }
}

auto SenVulkanBackend::LookupShader(SenShader handle) -> SenVulkanShaderEntry& {
    return _shaders.at(handle.ID);
}
