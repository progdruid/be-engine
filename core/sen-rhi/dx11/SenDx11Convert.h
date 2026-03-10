#pragma once
#include <d3d11.h>
#include <unordered_map>
#include <string>
#include <sen-rhi/SenTypes.h>
#include <umbrellas/include-libassert.h>

namespace Sen::Dx11 {

    inline auto ToFormat(SenFormat format) -> DXGI_FORMAT {
        switch (format) {
            case SenFormat::RGBA8_Unorm:     return DXGI_FORMAT_R8G8B8A8_UNORM;
            case SenFormat::RGBA16_Float:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case SenFormat::R11G11B10_Float: return DXGI_FORMAT_R11G11B10_FLOAT;
            case SenFormat::Depth32:         return DXGI_FORMAT_R32_TYPELESS;
            case SenFormat::RGB32_Float:     return DXGI_FORMAT_R32G32B32_FLOAT;
            case SenFormat::RGBA32_Float:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case SenFormat::RG32_Float:      return DXGI_FORMAT_R32G32_FLOAT;
            default:                         return DXGI_FORMAT_UNKNOWN;
        }
    }

    inline auto ToBindFlags(SenTextureUsage usage) -> uint32_t {
        uint32_t result = 0;
        if (HasAny(usage, SenTextureUsage::ShaderResource)) result |= D3D11_BIND_SHADER_RESOURCE;
        if (HasAny(usage, SenTextureUsage::RenderTarget))   result |= D3D11_BIND_RENDER_TARGET;
        if (HasAny(usage, SenTextureUsage::DepthStencil))   result |= D3D11_BIND_DEPTH_STENCIL;
        return result;
    }

    inline auto ToTopology(SenTopology topology) -> D3D11_PRIMITIVE_TOPOLOGY {
        switch (topology) {
            case SenTopology::TriangleList:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case SenTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            case SenTopology::LineList:      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            case SenTopology::PointList:     return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            case SenTopology::PatchList3:    return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
            default:                         return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        }
    }

    inline auto ToViewport(const SenViewport& vp) -> D3D11_VIEWPORT {
        return {
            .TopLeftX = vp.X,
            .TopLeftY = vp.Y,
            .Width    = vp.Width,
            .Height   = vp.Height,
            .MinDepth = vp.MinDepth,
            .MaxDepth = vp.MaxDepth,
        };
    }

    struct Dx11BufferAccessDesc {
        D3D11_USAGE usage;
        UINT        cpuAccessFlags;
    };

    inline auto ToBufferBindFlag(SenBufferUsage usage) -> UINT {
        switch (usage) {
            case SenBufferUsage::Vertex:   return D3D11_BIND_VERTEX_BUFFER;
            case SenBufferUsage::Index:    return D3D11_BIND_INDEX_BUFFER;
            case SenBufferUsage::Constant: return D3D11_BIND_CONSTANT_BUFFER;
            default:                       return 0;
        }
    }

    inline auto ToBufferAccess(SenBufferAccess access) -> Dx11BufferAccessDesc {
        switch (access) {
            case SenBufferAccess::Dynamic:   return { D3D11_USAGE_DYNAMIC,   D3D11_CPU_ACCESS_WRITE };
            case SenBufferAccess::Static:    return { D3D11_USAGE_DEFAULT,   0                      };
            case SenBufferAccess::Immutable: return { D3D11_USAGE_IMMUTABLE, 0                      };
            default:                         return { D3D11_USAGE_DEFAULT,   0                      };
        }
    }

    inline auto ToFilter(SenFilter filter, bool comparison) -> D3D11_FILTER {
        if (comparison) {
            switch (filter) {
                case SenFilter::Point:       return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
                case SenFilter::Linear:      return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
                case SenFilter::Anisotropic: return D3D11_FILTER_COMPARISON_ANISOTROPIC;
                default:                     return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            }
        }
        switch (filter) {
            case SenFilter::Point:       return D3D11_FILTER_MIN_MAG_MIP_POINT;
            case SenFilter::Linear:      return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            case SenFilter::Anisotropic: return D3D11_FILTER_ANISOTROPIC;
            default:                     return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        }
    }

    inline auto ToAddressMode(SenAddressMode address) -> D3D11_TEXTURE_ADDRESS_MODE {
        switch (address) {
            case SenAddressMode::Wrap:   return D3D11_TEXTURE_ADDRESS_WRAP;
            case SenAddressMode::Clamp:  return D3D11_TEXTURE_ADDRESS_CLAMP;
            case SenAddressMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
            default:                     return D3D11_TEXTURE_ADDRESS_CLAMP;
        }
    }

    inline auto ToVertexSemanticName(std::string_view semantic) -> const char* {
        static const std::unordered_map<std::string, const char*> Map = {
            {"position", "POSITION"},
            {"normal",   "NORMAL"},
            {"color3",   "COLOR"},
            {"color4",   "COLOR"},
            {"uv0",      "TEXCOORD"},
            {"uv1",      "TEXCOORD1"},
            {"uv2",      "TEXCOORD2"},
        };
        return Map.at(std::string(semantic));
    }

    inline auto ToBlendFactor(SenBlendFactor factor) -> D3D11_BLEND {
        switch (factor) {
            case SenBlendFactor::Zero:          return D3D11_BLEND_ZERO;
            case SenBlendFactor::One:           return D3D11_BLEND_ONE;
            case SenBlendFactor::SrcColor:      return D3D11_BLEND_SRC_COLOR;
            case SenBlendFactor::InvSrcColor:   return D3D11_BLEND_INV_SRC_COLOR;
            case SenBlendFactor::SrcAlpha:      return D3D11_BLEND_SRC_ALPHA;
            case SenBlendFactor::InvSrcAlpha:   return D3D11_BLEND_INV_SRC_ALPHA;
            case SenBlendFactor::DstColor:      return D3D11_BLEND_DEST_COLOR;
            case SenBlendFactor::InvDstColor:   return D3D11_BLEND_INV_DEST_COLOR;
            case SenBlendFactor::DstAlpha:      return D3D11_BLEND_DEST_ALPHA;
            case SenBlendFactor::InvDstAlpha:   return D3D11_BLEND_INV_DEST_ALPHA;
        }
        be_assert(false, "Unknown SenBlendFactor");
        return D3D11_BLEND_ONE;
    }

    inline auto ToBlendOp(SenBlendOp op) -> D3D11_BLEND_OP {
        switch (op) {
            case SenBlendOp::Add:              return D3D11_BLEND_OP_ADD;
            case SenBlendOp::Subtract:         return D3D11_BLEND_OP_SUBTRACT;
            case SenBlendOp::ReverseSubtract:  return D3D11_BLEND_OP_REV_SUBTRACT;
            case SenBlendOp::Min:              return D3D11_BLEND_OP_MIN;
            case SenBlendOp::Max:              return D3D11_BLEND_OP_MAX;
        }
        be_assert(false, "Unknown SenBlendOp");
        return D3D11_BLEND_OP_ADD;
    }

    inline auto ToCullMode(SenCullMode mode) -> D3D11_CULL_MODE {
        switch (mode) {
            case SenCullMode::None:   return D3D11_CULL_NONE;
            case SenCullMode::Front:  return D3D11_CULL_FRONT;
            case SenCullMode::Back:   return D3D11_CULL_BACK;
        }
        be_assert(false, "Unknown SenCullMode");
        return D3D11_CULL_BACK;
    }

    inline auto ToFillMode(SenFillMode mode) -> D3D11_FILL_MODE {
        switch (mode) {
            case SenFillMode::Solid:      return D3D11_FILL_SOLID;
            case SenFillMode::Wireframe:  return D3D11_FILL_WIREFRAME;
        }
        be_assert(false, "Unknown SenFillMode");
        return D3D11_FILL_SOLID;
    }

    inline auto ToComparisonFunc(SenComparisonFunc func) -> D3D11_COMPARISON_FUNC {
        switch (func) {
            case SenComparisonFunc::Never:         return D3D11_COMPARISON_NEVER;
            case SenComparisonFunc::Less:          return D3D11_COMPARISON_LESS;
            case SenComparisonFunc::Equal:         return D3D11_COMPARISON_EQUAL;
            case SenComparisonFunc::LessEqual:     return D3D11_COMPARISON_LESS_EQUAL;
            case SenComparisonFunc::Greater:       return D3D11_COMPARISON_GREATER;
            case SenComparisonFunc::NotEqual:      return D3D11_COMPARISON_NOT_EQUAL;
            case SenComparisonFunc::GreaterEqual:  return D3D11_COMPARISON_GREATER_EQUAL;
            case SenComparisonFunc::Always:        return D3D11_COMPARISON_ALWAYS;
        }
        be_assert(false, "Unknown SenComparisonFunc");
        return D3D11_COMPARISON_LESS;
    }

} // namespace Sen::Dx11
