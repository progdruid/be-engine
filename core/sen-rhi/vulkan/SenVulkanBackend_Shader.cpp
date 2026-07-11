#include "SenVulkanBackend.h"

#include <sen-rhi/SenShaderCompiler.h>
#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::CreateShader(const SenShaderSourceDesc& sourceDesc) -> SenShader {
    auto compileResult = SenShaderCompiler::Compile(sourceDesc.SourcePath, sourceDesc.FunctionName, sourceDesc.Stage);
    be_assert(compileResult, "SenVulkanBackend::CreateShader: shader compilation failed: " + compileResult.error());

    auto& spirv = compileResult.value();

    VkShaderModuleCreateInfo moduleInfo {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size() * sizeof(uint32_t),
        .pCode    = spirv.data(),
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
