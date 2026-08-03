#include "SenVulkanBackend.h"

#include <algorithm>

#include <sen-rhi/SenShaderCompiler.h>
#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::CreateShader(const SenShaderSourceDesc& sourceDesc) -> SenShader {
    auto compileResult = SenShaderCompiler::Compile(sourceDesc.SourcePath, sourceDesc.FunctionName, sourceDesc.Stage);
    be_assert(compileResult, "SenVulkanBackend::CreateShader: shader compilation failed: " + compileResult.error());

    auto& bytecode = compileResult.value().Bytecode;

    VkShaderModuleCreateInfo moduleInfo {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = bytecode.size() * sizeof(uint32_t),
        .pCode    = bytecode.data(),
    };

    const SenShader handle { _nextShaderId++ };
    auto& entry = _shaders[handle.ID];
    entry.Stage = sourceDesc.Stage;
    entry.SourcePath = std::filesystem::weakly_canonical(sourceDesc.SourcePath);
    entry.FunctionName = sourceDesc.FunctionName;
    entry.Includes = std::move(compileResult.value().Includes);

    VkResult result = vkCreateShaderModule(_device, &moduleInfo, nullptr, &entry.Module);
    be_assert(result == VK_SUCCESS, "Failed to create shader module!");

    return handle;
}

auto SenVulkanBackend::ReloadShader(SenShader handle) -> bool {
    auto& entry = _shaders.at(handle.ID);

    auto compileResult = SenShaderCompiler::Compile(entry.SourcePath, entry.FunctionName, entry.Stage);
    if (!compileResult) {
        std::fprintf(
            stderr, "[shader] reload failed: %s:%s\n%s\n",
            entry.SourcePath.filename().c_str(), entry.FunctionName.c_str(), compileResult.error().c_str()
        );
        return false;
    }

    auto& bytecode = compileResult.value().Bytecode;

    VkShaderModuleCreateInfo moduleInfo {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = bytecode.size() * sizeof(uint32_t),
        .pCode    = bytecode.data(),
    };

    VkShaderModule reloadedModule = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(_device, &moduleInfo, nullptr, &reloadedModule);
    if (result != VK_SUCCESS) {
        std::fprintf(stderr, "[shader] reload failed: %s: vkCreateShaderModule\n", entry.SourcePath.filename().c_str());
        return false;
    }

    vkDestroyShaderModule(_device, entry.Module, nullptr);
    entry.Module = reloadedModule;
    entry.Includes = std::move(compileResult.value().Includes);
    return true;
}

auto SenVulkanBackend::ReloadSources(std::span<const std::filesystem::path> paths) -> void {
    auto changed = std::vector<std::filesystem::path>();
    changed.reserve(paths.size());
    for (const auto& path : paths) {
        changed.push_back(std::filesystem::weakly_canonical(path));
    }

    auto contains = [&](const std::filesystem::path& path) -> bool {
        return std::ranges::find(changed, path) != changed.end();
    };

    auto affectedShaders = std::vector<SenShader>();
    for (const auto& [id, entry] : _shaders) {
        const bool touched = contains(entry.SourcePath)
            || std::ranges::any_of(entry.Includes, contains);
        if (touched) {
            affectedShaders.push_back(SenShader { id });
        }
    }

    if (affectedShaders.empty()) {
        return;
    }

    WaitIdle();

    auto reloadedShaders = std::vector<SenShader>();
    for (auto handle : affectedShaders) {
        if (ReloadShader(handle)) {
            reloadedShaders.push_back(handle);
        }
    }

    if (reloadedShaders.empty()) {
        return;
    }

    auto reloadedPipelineCount = size_t(0);
    for (auto& [id, entry] : _pipelines) {
        const auto& desc = entry.Desc;
        const bool uses = std::ranges::any_of(
            reloadedShaders,
            [&](SenShader shader) -> bool {
                return desc.VertexShader.ID == shader.ID
                    || desc.HullShader.ID == shader.ID
                    || desc.DomainShader.ID == shader.ID
                    || desc.PixelShader.ID == shader.ID
                    || desc.ComputeShader.ID == shader.ID;
            }
        );
        if (uses) {
            ReloadPipeline(SenPipeline { id });
            ++reloadedPipelineCount;
        }
    }

    std::fprintf(
        stderr, "[shader] reloaded %zu shaders, %zu pipelines\n",
        reloadedShaders.size(), reloadedPipelineCount
    );
}

auto SenVulkanBackend::DestroyShader(SenShader handle) -> void {
    const auto it = _shaders.find(handle.ID);
    if (it != _shaders.end()) {
        vkDestroyShaderModule(_device, it->second.Module, nullptr);
        _shaders.erase(it);
    }
}

auto SenVulkanBackend::LookupShader(SenShader handle) -> SenVulkanShaderEntry& {
    return _shaders.at(handle.ID);
}
