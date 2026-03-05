#pragma once
#include <d3d11.h>
#include <sen-rhi/SenTypes.h>
#include <sen-rhi/SenSampler.h>

namespace Sen::Dx11 {

    inline auto ToFormat(SenFormat format) -> DXGI_FORMAT {
        switch (format) {
            case SenFormat::RGBA8_Unorm:     return DXGI_FORMAT_R8G8B8A8_UNORM;
            case SenFormat::RGBA16_Float:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case SenFormat::R11G11B10_Float: return DXGI_FORMAT_R11G11B10_FLOAT;
            case SenFormat::Depth32:         return DXGI_FORMAT_R32_TYPELESS;
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
            case SenBufferAccess::Default:   return { D3D11_USAGE_DEFAULT,   0                      };
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

} // namespace Sen::Dx11
