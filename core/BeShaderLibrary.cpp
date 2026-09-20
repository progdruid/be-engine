#include "BeShaderLibrary.h"

#include <sstream>
#include <fstream>
#include <ranges>

#include "BeFileWatcher.h"
#include "BeMesh.h"
#include "BeShader.h"
#include "BeShaderCompiler.h"
#include "BeShaderTools.h"
#include "BeTexture.h"
#include "sen-rhi/SenBackend.h"

namespace {
    auto ParseCullMode(const std::string& str) -> SenCullMode {
        if (str == "none") return SenCullMode::None;
        if (str == "front") return SenCullMode::Front;
        if (str == "back") return SenCullMode::Back;
        be_assert(false, "Unknown cull mode: " + str);
        return SenCullMode::Back;
    }

    auto ParseFillMode(const std::string& str) -> SenFillMode {
        if (str == "solid") return SenFillMode::Solid;
        if (str == "wireframe") return SenFillMode::Wireframe;
        be_assert(false, "Unknown fill mode: " + str);
        return SenFillMode::Solid;
    }

    auto ParseRasterizerString(const std::string& str) -> SenRasterizerState {
        auto state = SenRasterizerState();
        const auto parts = BeShaderTools::Split(str, "-");
        be_assert(!parts.empty(), "Invalid rasterizer state format: " + str);

        state.CullMode = ParseCullMode(std::string(parts[0]));
        if (parts.size() > 1) {
            state.FillMode = ParseFillMode(std::string(parts[1]));
        }
        return state;
    }

    auto ParseBlendString(const std::string& str) -> SenBlendState {
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

    auto ParseComparisonFunc(const std::string& str) -> SenComparisonFunc {
        if (str == "never") return SenComparisonFunc::Never;
        if (str == "less") return SenComparisonFunc::Less;
        if (str == "equal") return SenComparisonFunc::Equal;
        if (str == "less-equal") return SenComparisonFunc::LessEqual;
        if (str == "greater") return SenComparisonFunc::Greater;
        if (str == "not-equal") return SenComparisonFunc::NotEqual;
        if (str == "greater-equal") return SenComparisonFunc::GreaterEqual;
        if (str == "always") return SenComparisonFunc::Always;
        be_assert(false, "Unknown comparison func: " + str);
        return SenComparisonFunc::Less;
    }

    auto ParseDepthStencilString(const std::string& str) -> SenDepthStencilState {
        auto state = SenDepthStencilState();
        if (str == "disable") {
            state.DepthEnable = false;
            return state;
        }
        state.DepthEnable = true;
        state.DepthFunc = ParseComparisonFunc(str);
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
        [] { return SenBackend::GetShaderSourcePaths(); },
        [](std::span<const std::filesystem::path> changed) { SenBackend::ReloadSources(changed); }
    );
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
        shader->ShaderCompute = SenBackend::CreateShader({
            .SourcePath = filePath,
            .FunctionName = meta.ComputeFn,
            .Stage = SenShaderStage::Compute,
        });
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
        shader->RasterizerState = ParseRasterizerString(meta.Rasterizer);
    }
    if (!meta.Blend.empty()) {
        shader->BlendState = ParseBlendString(meta.Blend);
    }
    if (!meta.Depth.empty()) {
        shader->DepthStencilState = ParseDepthStencilString(meta.Depth);
    }

    if (!meta.VertexFn.empty()) {
        shader->ShaderType = BeShaderType::Vertex;
        shader->ShaderVertex = SenBackend::CreateShader({
            .SourcePath = filePath,
            .FunctionName = meta.VertexFn,
            .Stage = SenShaderStage::Vertex,
        });

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
        shader->ShaderHull = SenBackend::CreateShader({
            .SourcePath = filePath,
            .FunctionName = meta.HullFn,
            .Stage = SenShaderStage::Hull,
        });
        shader->ShaderDomain = SenBackend::CreateShader({
            .SourcePath = filePath,
            .FunctionName = meta.DomainFn,
            .Stage = SenShaderStage::Domain,
        });
    }

    if (!meta.PixelFn.empty()) {
        be_assert(!meta.Targets.empty(), "", filePath);
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;
        shader->ShaderPixel = SenBackend::CreateShader({
            .SourcePath = filePath,
            .FunctionName = meta.PixelFn,
            .Stage = SenShaderStage::Pixel,
        });

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
        SenTextureUsage Usage = SenTextureUsage::ShaderResource;
        bool Cubemap = false;
        uint32_t ArrayLength = 1;
    };

    const BuiltinDefaultTexture builtins[] = {
        { "white", glm::vec4(1.f) },
        { "black", glm::vec4(0.f, 0.f, 0.f, 1.f) },
        { "storage-black", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::ShaderResource | SenTextureUsage::Storage },
        { "black-cube", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::ShaderResource, true },
        { "black-array", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::ShaderResource, false, 2 },
        { "black-cube-array", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::ShaderResource, true, 2 },
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

    auto sampler = SenBackend::CreateSampler({
        .Filter     = filter,
        .Address    = address,
        .Comparison = hasComparison,
    });

    _samplers[key] = sampler;
    return sampler;
}

