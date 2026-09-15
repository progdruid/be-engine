#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>
#include "umbrellas/include-libassert.h"
#include "BeMaterialScheme.h"
#include "BeShaderTools.h"

struct BeShader;
class BeTexture;

class BeShaderLibrary {

    hide
    static std::unordered_map<std::filesystem::path, std::string> _shaderSources;
    static std::unordered_map<std::string, std::unique_ptr<BeShader>> _shaders;
    static std::unordered_map<std::string, BeMaterialScheme> _materialSchemes;
    static std::unordered_map<std::string, std::shared_ptr<BeTexture>> _defaultTextures;
    static std::unordered_map<std::string, SenSampler> _samplers;
    static uint32_t _shaderCount;

    expose // lifecycle
    static auto Shutdown() -> void;

    expose // shaders + schemes
    static auto LoadShaderFiles(const std::vector<std::filesystem::path>& filePaths) -> void;
    static auto LoadShaderDirectory(const std::filesystem::path& dir) -> void;
    static auto LoadShaders() -> void { LoadShaderDirectory("shaders/"); }

    static auto GetShader(std::string_view name) -> raw_ptr<BeShader>;
    static auto HasShader(std::string_view name) -> bool { return _shaders.contains(std::string(name)); }

    static auto GetMaterialScheme(std::string_view name) -> const BeMaterialScheme&;
    static auto HasMaterialScheme(std::string_view name) -> bool { return _materialSchemes.contains(std::string(name)); }

    static auto GetShaderScheme(const BeShader& shader, std::string_view link) -> const BeMaterialScheme&;

    hide
    static auto CreateShader(const BeShaderTools::ParsedShader& meta) -> std::unique_ptr<BeShader>;

    expose // default textures + samplers
    static auto RegisterBuiltinDefaultTextures() -> void;
    static auto RegisterDefaultTexture(std::string_view name, std::shared_ptr<BeTexture> texture) -> void;
    static auto GetDefaultTexture(std::string_view name) -> std::weak_ptr<BeTexture> { be_assert(_defaultTextures.contains(std::string(name))); return _defaultTextures.at(std::string(name)); }
    static auto HasDefaultTexture(std::string_view name) -> bool { return _defaultTextures.contains(std::string(name)); }
    static auto GetSampler(std::string_view samplerDescString) -> SenSampler;
};
