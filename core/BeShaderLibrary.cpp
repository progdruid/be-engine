#include "BeShaderLibrary.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <ranges>
#include <filesystem>

#include "BeBackend.h"
#include "BeFileWatcher.h"
#include "BeMesh.h"
#include "BeShader.h"
#include "BeShaderCompiler.h"
#include "BeShaderTools.h"
#include "BeTexture.h"
#include "Sen.h"

namespace {
    auto ParseCull(const std::string& str) -> SenCull {
        if (str == "none") return SenCull::None;
        if (str == "front") return SenCull::Front;
        if (str == "back") return SenCull::Back;
        be_assert(false, "Unknown cull mode: " + str);
        return SenCull::Back;
    }

    auto ParseFill(const std::string& str) -> SenFill {
        if (str == "solid") return SenFill::Solid;
        if (str == "wireframe") return SenFill::Wireframe;
        be_assert(false, "Unknown fill mode: " + str);
        return SenFill::Solid;
    }

    auto ParseBlend(const std::string& str) -> SenBlendState {
        auto state = SenBlendState();
        if (str == "disable") {
            state.Enable = false;
            return state;
        }
        if (str == "alpha") {
            state.Enable = true;
            state.SrcBlend = SenBlendFactor::SrcAlpha;
            state.DstBlend = SenBlendFactor::InvSrcAlpha;
            state.BlendOp = SenBlendOp::Add;
            return state;
        }
        if (str == "additive") {
            state.Enable = true;
            state.SrcBlend = SenBlendFactor::One;
            state.DstBlend = SenBlendFactor::One;
            state.BlendOp = SenBlendOp::Add;
            return state;
        }
        if (str == "multiply") {
            state.Enable = true;
            state.SrcBlend = SenBlendFactor::DstColor;
            state.DstBlend = SenBlendFactor::Zero;
            state.BlendOp = SenBlendOp::Add;
            return state;
        }
        be_assert(false, "Unknown blend preset: " + str);
        return state;
    }

    auto ParseCompare(const std::string& str) -> SenCompare {
        if (str == "never") return SenCompare::Never;
        if (str == "less") return SenCompare::Less;
        if (str == "equal") return SenCompare::Equal;
        if (str == "less-equal") return SenCompare::LessEqual;
        if (str == "greater") return SenCompare::Greater;
        if (str == "not-equal") return SenCompare::NotEqual;
        if (str == "greater-equal") return SenCompare::GreaterEqual;
        if (str == "always") return SenCompare::Always;
        be_assert(false, "Unknown comparison func: " + str);
        return SenCompare::Less;
    }

    auto ParseDepth(const std::string& str) -> SenDepthState {
        auto state = SenDepthState();
        if (str == "disable") {
            state.Test = false;
            return state;
        }
        state.Test = true;
        state.Compare = ParseCompare(str);
        return state;
    }
}

std::unordered_map<std::filesystem::path, std::string>          BeShaderLibrary::_shaderSources;
std::unordered_map<std::string, std::unique_ptr<BeShader>>      BeShaderLibrary::_shaders;
std::unordered_map<std::string, BeMaterialScheme>              BeShaderLibrary::_materialSchemes;
std::unordered_map<std::string, std::shared_ptr<BeTexture>>    BeShaderLibrary::_defaultTextures;
std::unordered_map<std::string, SenSampler>                    BeShaderLibrary::_samplers;
uint32_t BeShaderLibrary::_shaderCount = 0;


auto BeShaderLibrary::Init() -> void {
    BeShaderCompiler::Launch();

    BeFileWatcher::Register(
        [] { return GetSourcePaths(); },
        [](std::span<const std::filesystem::path> changed) { ReloadSources(changed); }
    );
}

auto BeShaderLibrary::GetSourcePaths() -> std::vector<std::filesystem::path> {
    auto paths = std::vector<std::filesystem::path>();
    for (const auto& shader : _shaders | std::views::values) {
        paths.push_back(shader->SourcePath);
        for (const auto& include : shader->Includes) {
            paths.push_back(include);
        }
    }
    std::ranges::sort(paths);
    paths.erase(std::ranges::unique(paths).begin(), paths.end());
    return paths;
}

auto BeShaderLibrary::ReloadSources(std::span<const std::filesystem::path> changed) -> void {
    auto canonical = std::vector<std::filesystem::path>();
    canonical.reserve(changed.size());
    for (const auto& path : changed) {
        canonical.push_back(std::filesystem::weakly_canonical(path));
    }

    auto touched = [&](const std::filesystem::path& path) -> bool {
        return std::ranges::find(canonical, path) != canonical.end();
    };

    auto reloaded = std::vector<raw_ptr<BeShader>>();
    for (const auto& shader : _shaders | std::views::values) {
        if (!touched(shader->SourcePath) && !std::ranges::any_of(shader->Includes, touched)) {
            continue;
        }
        if (RecompileShader(*shader)) {
            reloaded.push_back(shader.get());
        }
    }

    if (reloaded.empty()) {
        return;
    }

    Sen::WaitIdle();

    auto pipelineCount = uint32_t(0);
    for (const auto shader : reloaded) {
        pipelineCount += BeBackend::RebuildPipelines(*shader);
    }

    std::fprintf(
        stderr, "[shader] reloaded %zu shaders, %u pipelines\n",
        reloaded.size(), pipelineCount
    );
}

auto BeShaderLibrary::RecompileShader(BeShader& shader) -> bool {
    auto stages = std::array<raw_ptr<BeShaderStageCode>, 5> {
        &shader.StageVertex, &shader.StageHull, &shader.StageDomain, &shader.StagePixel, &shader.StageCompute
    };
    auto stageKinds = std::array<BeShaderStage, 5> {
        BeShaderStage::Vertex, BeShaderStage::Hull, BeShaderStage::Domain, BeShaderStage::Pixel, BeShaderStage::Compute
    };

    auto bytecodes = std::array<std::vector<uint32_t>, 5>();
    auto includes = std::vector<std::filesystem::path>();

    for (size_t i = 0; i < stages.size(); ++i) {
        if (!stages[i]->IsValid()) {
            continue;
        }
        auto result = BeShaderCompiler::Compile(shader.SourcePath, stages[i]->FunctionName, stageKinds[i]);
        if (!result) {
            std::fprintf(
                stderr, "[shader] reload failed: %s:%s\n%s\n",
                shader.SourcePath.filename().c_str(), stages[i]->FunctionName.c_str(), result.error().c_str()
            );
            return false;
        }
        bytecodes[i] = std::move(result.value().Bytecode);
        for (auto& include : result.value().Includes) {
            if (std::ranges::find(includes, include) == includes.end()) {
                includes.push_back(std::move(include));
            }
        }
    }

    for (size_t i = 0; i < stages.size(); ++i) {
        if (stages[i]->IsValid()) {
            stages[i]->Bytecode = std::move(bytecodes[i]);
        }
    }
    shader.Includes = std::move(includes);
    return true;
}

auto BeShaderLibrary::Shutdown() -> void {
    _shaders.clear();
    _materialSchemes.clear();
    _shaderSources.clear();
    _defaultTextures.clear();
    _samplers.clear();
}

auto BeShaderLibrary::LoadShaderFiles(const std::vector<std::filesystem::path>& filePaths) -> void {

    // collect sources
    auto sourcesToIndex = std::vector<std::pair<std::filesystem::path, std::string>>();
    for (const auto& path : filePaths) {
        if (_shaderSources.contains(path))
            continue;

        be_assert(std::filesystem::exists(path), path);

        auto file = std::ifstream(path);
        auto buffer = std::stringstream();
        buffer << file.rdbuf();
        auto src = buffer.str();

        _shaderSources[path] = src;
        sourcesToIndex.emplace_back(path, src);
    }

    // parse every source file once
    auto parsedFiles = std::vector<std::pair<std::filesystem::path, BeShaderTools::ParsedShaderFile>>();
    for (const auto& [path, src] : sourcesToIndex) {
        auto parsed = BeShaderTools::ParseShaderFile(src, path);
        be_assert(parsed.has_value(), parsed.error());
        parsedFiles.emplace_back(path, std::move(*parsed));
    }

    // index material schemes (all, before any shader resolves its binds)
    auto parsedMaterials = std::unordered_map<std::string, const BeShaderTools::ParsedMaterial*>();
    for (const auto& parsed : parsedFiles | std::views::values) {
        for (const auto& material : parsed.Materials) {
            _materialSchemes[material.Name] = BeMaterialScheme::Create(material.Name, material.Properties);
            parsedMaterials[material.Name] = &material;
        }
    }

    // link + index shaders
    for (auto& parsed : parsedFiles | std::views::values) {
        if (!parsed.Shader) {
            continue;
        }
        auto boundMaterials = std::vector<BeShaderTools::ParsedMaterial>();
        for (const auto& bind : parsed.Shader->Binds) {
            boundMaterials.push_back(*parsedMaterials.at(bind.Scheme));
        }
        parsed.Shader->Root = BeShaderTools::BuildRootLayout(*parsed.Shader, boundMaterials);

        auto shader = CreateShader(*parsed.Shader);
        auto name = shader->Name;
        _shaders[std::move(name)] = std::move(shader);
    }
}

auto BeShaderLibrary::LoadShaderDirectory(const std::filesystem::path& dir) -> void {
    be_assert(std::filesystem::exists(dir), dir);

    BeShaderCompiler::AddSearchPath(dir);

    auto filePaths = std::vector<std::filesystem::path>();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".hlsl") {
            filePaths.push_back(entry.path());
        }
    }

    LoadShaderFiles(filePaths);
}

auto BeShaderLibrary::GetShader(std::string_view name) -> raw_ptr<BeShader> {
    be_assert(_shaders.contains(std::string(name)), name);
    return _shaders.at(std::string(name)).get();
}

auto BeShaderLibrary::GetMaterialScheme(std::string_view name) -> const BeMaterialScheme& {
    be_assert(_materialSchemes.contains(std::string(name)), name);
    return _materialSchemes.at(std::string(name));
}

auto BeShaderLibrary::GetShaderScheme(const BeShader& shader, std::string_view link) -> const BeMaterialScheme& {
    for (const auto& entry : shader.MaterialSchemes) {
        if (entry.Link == link) {
            return entry.Scheme;
        }
    }
    be_assert(false, "BeShaderLibrary: shader has no material scheme under link", shader.Name, link);
    return shader.MaterialSchemes[0].Scheme;
}

auto BeShaderLibrary::CreateShader(const BeShaderTools::ParsedShader& meta) -> std::unique_ptr<BeShader> {
    const auto& filePath = meta.SourceFile;
    be_assert(
        std::filesystem::exists(filePath), 
        "Shader file doesn't exist: " + filePath.string()
    );
    be_assert(
        meta.Root.has_value(), 
        "BeShaderLibrary::CreateShader: shader not linked (no root layout)", 
        meta.Name
    );

    auto shader = std::make_unique<BeShader>();
    shader->ShaderID = ++_shaderCount;
    shader->Name = meta.Name;
    shader->RootLayout = *meta.Root;
    shader->SourcePath = std::filesystem::weakly_canonical(filePath);

    auto compileStage = [&](BeShaderStage stage, const std::string& functionName, BeShaderStageCode& code) -> void {
        auto result = BeShaderCompiler::Compile(filePath, functionName, stage);
        if (!result) {
            be_assert(false, "BeShaderLibrary::CreateShader: compilation failed", filePath, functionName, result.error());
            return;
        }
        code.FunctionName = functionName;
        code.Bytecode = std::move(result.value().Bytecode);
        for (auto& include : result.value().Includes) {
            if (std::ranges::find(shader->Includes, include) == shader->Includes.end()) {
                shader->Includes.push_back(std::move(include));
            }
        }
    };

    if (!meta.Binds.empty()) {
        shader->HasMaterial = true;
        for (const auto& bind : meta.Binds) {
            auto& entry = shader->MaterialSchemes.emplace_back();
            entry.Link = bind.Link;
            entry.Scheme = GetMaterialScheme(bind.Scheme);
            entry.Index = bind.Slot;
        }
    }

    if (!meta.ComputeFn.empty()) {
        shader->ShaderType = BeShaderType::Compute;
        compileStage(BeShaderStage::Compute, meta.ComputeFn, shader->StageCompute);
        return shader;
    }

    be_assert(!meta.Topology.empty(), "", filePath);
    if (meta.Topology == "triangle-list") {
        shader->Topology = SenTopology::TriangleList;
    } else if (meta.Topology == "triangle-strip") {
        shader->Topology = SenTopology::TriangleStrip;
    } else if (meta.Topology == "patch-list-3") {
        shader->Topology = SenTopology::PatchList3;
    } else {
        be_assert(false, "Unsupported topology", filePath);
    }

    if (!meta.Rasterizer.empty()) {
        const auto parts = BeShaderTools::Split(meta.Rasterizer, "-");
        be_assert(!parts.empty(), "Invalid rasterizer state format: " + meta.Rasterizer);

        shader->Cull = ParseCull(std::string(parts[0]));
        if (parts.size() > 1) {
            shader->Fill = ParseFill(std::string(parts[1]));
        }
    }
    if (!meta.Blend.empty()) {
        shader->BlendState = ParseBlend(meta.Blend);
    }
    if (!meta.Depth.empty()) {
        shader->DepthState = ParseDepth(meta.Depth);
    }

    if (!meta.VertexFn.empty()) {
        shader->ShaderType = BeShaderType::Vertex;
        compileStage(BeShaderStage::Vertex, meta.VertexFn, shader->StageVertex);

        if (!meta.VertexLayout.empty()) {
            static const std::unordered_map<std::string, SenFormat> ElementFormats = {
                { "position", SenFormat::RGB32_Float },
                { "normal", SenFormat::RGB32_Float },
                { "color3", SenFormat::RGB32_Float },
                { "color4", SenFormat::RGBA32_Float },
                { "uv0", SenFormat::RG32_Float },
                { "tangent", SenFormat::RGBA32_Float },
            };
            static const std::unordered_map<std::string, uint32_t> ElementOffsets = {
                { "position", 0 },
                { "normal", 12 },
                { "color3", 24 },
                { "color4", 24 },
                { "uv0", 40 },
                { "tangent", 48 },
            };

            uint32_t location = 0;
            for (const auto& semantic : meta.VertexLayout) {
                shader->VertexLayout.push_back({
                    .Semantic = semantic,
                    .Location = location,
                    .Format = ElementFormats.at(semantic),
                    .Offset = ElementOffsets.at(semantic),
                });
                ++location;
            }
            shader->VertexStride = sizeof(BeFullVertex);
        }
    }

    if (!meta.HullFn.empty() || !meta.DomainFn.empty()) {
        shader->ShaderType = shader->ShaderType | BeShaderType::Tesselation;
        compileStage(BeShaderStage::Hull, meta.HullFn, shader->StageHull);
        compileStage(BeShaderStage::Domain, meta.DomainFn, shader->StageDomain);
    }

    if (!meta.PixelFn.empty()) {
        be_assert(!meta.Targets.empty(), "", filePath);
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;
        compileStage(BeShaderStage::Pixel, meta.PixelFn, shader->StagePixel);

        for (const auto& target : meta.Targets) {
            const uint32_t targetSlot = target.Slot;
            be_assert(!shader->PixelTargets.contains(target.Name), "", filePath);
            be_assert(!shader->PixelTargetsInverse.contains(targetSlot), "", filePath);
            shader->PixelTargets[target.Name] = targetSlot;
            shader->PixelTargetsInverse[targetSlot] = target.Name;
        }
    }

    return shader;
}

auto BeShaderLibrary::RegisterBuiltinDefaultTextures() -> void {

    struct BuiltinDefaultTexture {
        std::string_view Name;
        glm::vec4 Color;
        SenTextureUsage Usage = SenTextureUsage::Sampled;
        bool Cubemap = false;
        uint32_t ArrayLength = 1;
    };

    const BuiltinDefaultTexture builtins[] = {
        { "white", glm::vec4(1.f) },
        { "black", glm::vec4(0.f, 0.f, 0.f, 1.f) },
        { "storage-black", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::Sampled | SenTextureUsage::Storage },
        { "black-cube", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::Sampled, true },
        { "black-array", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::Sampled, false, 2 },
        { "black-cube-array", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::Sampled, true, 2 },
        { "default-orm", glm::vec4(0.f, 1.f, 1.f, 1.f) },
        { "flat-normal", glm::vec4(0.5f, 0.5f, 1.f, 1.f) },
    };

    for (const auto& builtin : builtins) {
        RegisterDefaultTexture(
            builtin.Name,
            BeTexture::Create(std::string(builtin.Name))
            .SetSize(1, 1)
            .SetCubemap(builtin.Cubemap)
            .SetArrayLength(builtin.ArrayLength)
            .SetUsage(builtin.Usage)
            .SetFormat(SenFormat::RGBA8_Unorm)
            .FillWithColor(builtin.Color)
            .Build()
        );
    }
}

auto BeShaderLibrary::RegisterDefaultTexture(std::string_view name, std::shared_ptr<BeTexture> texture) -> void {
    _defaultTextures[std::string(name)] = std::move(texture);
}

auto BeShaderLibrary::GetSampler(std::string_view samplerDescString) -> SenSampler {

    const auto key = std::string(samplerDescString);

    if (_samplers.contains(key)) {
        return _samplers[key];
    }

    auto tokens = BeShaderTools::Split(samplerDescString, "-");
    be_assert(
        tokens.size() == 2 || tokens.size() == 3,
        "Invalid samplerDescString. Expected format: filter-address[-cmp]",
        samplerDescString,
        tokens.size()
    );

    auto filterToken   = std::string(tokens[0]);
    auto addressToken  = std::string(tokens[1]);
    auto hasComparison = tokens.size() == 3 && tokens[2] == "cmp";

    SenFilter filter = SenFilter::Linear;
    if (filterToken == "point") {
        filter = SenFilter::Point;
    } else if (filterToken == "linear") {
        filter = SenFilter::Linear;
    } else if (filterToken == "anisotropic") {
        filter = SenFilter::Anisotropic;
    } else {
        be_assert(false, "Unknown filter token", filterToken);
    }

    SenAddressMode address = SenAddressMode::Clamp;
    if (addressToken == "wrap") {
        address = SenAddressMode::Wrap;
    } else if (addressToken == "clamp") {
        address = SenAddressMode::Clamp;
    } else if (addressToken == "mirror") {
        address = SenAddressMode::Mirror;
    } else {
        be_assert(false, "Unknown address token", addressToken);
    }

    auto sampler = Sen::CreateSampler({
        .Filter     = filter,
        .Address    = address,
        .Comparison = hasComparison,
    });
    BeBackend::RegisterSampler(sampler);

    _samplers[key] = sampler;
    return sampler;
}

