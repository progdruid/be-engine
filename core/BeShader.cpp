#include "BeShader.h"

#include "BeRenderer.h"
#include "BeMesh.h"
#include "BeShaderTools.h"
#include <umbrellas/include-libassert.h>

#include <sen-rhi/SenTypes.h>

#include "BeShaderLibrary.h"
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

    auto ParseBlendFactor(const std::string& str) -> SenBlendFactor {
        if (str == "zero") return SenBlendFactor::Zero;
        if (str == "one") return SenBlendFactor::One;
        if (str == "src-color") return SenBlendFactor::SrcColor;
        if (str == "inv-src-color") return SenBlendFactor::InvSrcColor;
        if (str == "src-alpha") return SenBlendFactor::SrcAlpha;
        if (str == "inv-src-alpha") return SenBlendFactor::InvSrcAlpha;
        if (str == "dst-color") return SenBlendFactor::DstColor;
        if (str == "inv-dst-color") return SenBlendFactor::InvDstColor;
        if (str == "dst-alpha") return SenBlendFactor::DstAlpha;
        if (str == "inv-dst-alpha") return SenBlendFactor::InvDstAlpha;
        be_assert(false, "Unknown blend factor: " + str);
        return SenBlendFactor::One;
    }

    auto ParseBlendOp(const std::string& str) -> SenBlendOp {
        if (str == "add") return SenBlendOp::Add;
        if (str == "subtract") return SenBlendOp::Subtract;
        if (str == "reverse-subtract") return SenBlendOp::ReverseSubtract;
        if (str == "min") return SenBlendOp::Min;
        if (str == "max") return SenBlendOp::Max;
        be_assert(false, "Unknown blend op: " + str);
        return SenBlendOp::Add;
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
}  // namespace


uint32_t BeShader::_shaderCount = 0;
auto BeShader::Create(const std::filesystem::path& filePath) -> std::unique_ptr<BeShader> {
    be_assert(
        std::filesystem::exists(filePath),
        "Shader file doesn't exist: " + filePath.string()
    );

    auto shader = std::make_unique<BeShader>();
    shader->ShaderID = ++_shaderCount;

    auto src = BeShaderTools::ReadFile(filePath);
    auto parsed = BeShaderTools::ParseShader(src);
    be_assert(parsed.has_value(), "Failed to parse @be-shader", filePath, parsed.error());
    const auto& meta = parsed.value();
    shader->Name = meta.Name;

    if (!meta.Binds.empty()) {
        shader->HasMaterial = true;

        shader->_pipelineDesc.BindGroupLayouts.resize(meta.Binds.size());
        for (const auto& bind : meta.Binds) {
            auto& entry = shader->_materialSchemes.emplace_back();
            entry.Link   = bind.Link;
            entry.Scheme = BeShaderLibrary::GetMaterialScheme(bind.Scheme);
            entry.Index  = bind.Slot;

            shader->_pipelineDesc.BindGroupLayouts[entry.Index] = entry.Scheme.BindGroupLayout;
        }
    }

    if (!meta.ComputeFn.empty()) {
        shader->ShaderType = BeShaderType::Compute;
        shader->ShaderCompute = SenBackend::CreateShader({
            .SourcePath   = filePath,
            .FunctionName = meta.ComputeFn,
            .Stage        = SenShaderStage::Compute,
        });
        shader->_pipelineDesc.ComputeShader = shader->ShaderCompute;
        return shader;
    }

    {
        be_assert(!meta.Topology.empty(), "", filePath);
        const auto& topology = meta.Topology;

        if (topology == "triangle-list") {
            shader->Topology = SenTopology::TriangleList;
        }
        else if (topology == "triangle-strip") {
            shader->Topology = SenTopology::TriangleStrip;
        }
        else if (topology == "patch-list-3") {
            shader->Topology = SenTopology::PatchList3;
        }
        else {
            be_assert(false, "Unsupported topology", filePath);
        }
    }

    // Initialize pipeline desc with defaults
    shader->_pipelineDesc.Topology = shader->Topology;
    shader->_pipelineDesc.RasterizerState.CullMode = SenCullMode::Back;
    shader->_pipelineDesc.BlendState.Enable = false;
    shader->_pipelineDesc.DepthStencilState.DepthEnable = true;

    // Parse render state
    if (!meta.Rasterizer.empty()) {
        shader->_pipelineDesc.RasterizerState = ParseRasterizerString(meta.Rasterizer);
    }

    if (!meta.Blend.empty()) {
        shader->_pipelineDesc.BlendState = ParseBlendString(meta.Blend);
    }

    if (!meta.Depth.empty()) {
        shader->_pipelineDesc.DepthStencilState = ParseDepthStencilString(meta.Depth);
    }

    auto shaderErrMsg = [&](const std::string& err, const std::string& stage) -> std::string {
        return
            "1. Shader compilation error.\n"
            "2. Path to shader: " + filePath.string() + "\n"
            "3. Shader stage that failed: " + stage + "\n"
            "4. Compiler output:\n" + err + "\n"
            "5. Source code:\n\n" + src;
    };

    if (!meta.VertexFn.empty()) {
        shader->ShaderType = BeShaderType::Vertex;

        shader->ShaderVertex = SenBackend::CreateShader({
            .SourcePath = filePath,
            .FunctionName = meta.VertexFn,
            .Stage = SenShaderStage::Vertex,
        });

        shader->_pipelineDesc.VertexShader = shader->ShaderVertex;

        if (!meta.VertexLayout.empty()) {
            static const std::unordered_map<std::string, SenFormat> ElementFormats = {
                {"position", SenFormat::RGB32_Float},
                {"normal",   SenFormat::RGB32_Float},
                {"color3",   SenFormat::RGB32_Float},
                {"color4",   SenFormat::RGBA32_Float},
                {"uv0",      SenFormat::RG32_Float},
                {"tangent",  SenFormat::RGBA32_Float},
            };
            static const std::unordered_map<std::string, uint32_t> ElementOffsets = {
                {"position", 0},
                {"normal",  12},
                {"color3",  24},
                {"color4",  24},
                {"uv0",     40},
                {"tangent", 48},
            };

            std::vector<SenVertexLayoutElement> layoutElements;
            uint32_t location = 0;
            for (const auto& semantic : meta.VertexLayout) {
                layoutElements.push_back({
                    .Semantic = semantic,
                    .Location = location,
                    .Format = ElementFormats.at(semantic),
                    .Offset = ElementOffsets.at(semantic),
                });
                location++;
            }

            shader->_pipelineDesc.VertexLayout = layoutElements;
            shader->_pipelineDesc.VertexStride = sizeof(BeFullVertex);
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

        shader->_pipelineDesc.HullShader = shader->ShaderHull;
        shader->_pipelineDesc.DomainShader = shader->ShaderDomain;
    }

    if (!meta.PixelFn.empty()) {
        be_assert(!meta.Targets.empty(), "", filePath);
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;

        shader->ShaderPixel = SenBackend::CreateShader({
            .SourcePath = filePath,
            .FunctionName = meta.PixelFn,
            .Stage = SenShaderStage::Pixel,
        });

        shader->_pipelineDesc.PixelShader = shader->ShaderPixel;

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
