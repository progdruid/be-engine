#pragma once
#include "GalTypes.h"

#ifdef _WIN32
#include <d3d11.h>

namespace GalFormatConverter {

inline auto ToDxgiFormat(GalFormat format) -> DXGI_FORMAT {
    switch (format) {
        case GalFormat::R8G8B8A8_UNORM:         return DXGI_FORMAT_R8G8B8A8_UNORM;
        case GalFormat::R8G8B8A8_UNORM_SRGB:    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case GalFormat::R16G16B16A16_FLOAT:      return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case GalFormat::R32G32B32A32_FLOAT:      return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case GalFormat::R32G32B32_FLOAT:         return DXGI_FORMAT_R32G32B32_FLOAT;
        case GalFormat::R32G32_FLOAT:            return DXGI_FORMAT_R32G32_FLOAT;
        case GalFormat::R11G11B10_FLOAT:         return DXGI_FORMAT_R11G11B10_FLOAT;
        case GalFormat::R32_FLOAT:               return DXGI_FORMAT_R32_FLOAT;
        case GalFormat::R16_FLOAT:               return DXGI_FORMAT_R16_FLOAT;
        case GalFormat::R32_TYPELESS:            return DXGI_FORMAT_R32_TYPELESS;
        case GalFormat::R24G8_TYPELESS:          return DXGI_FORMAT_R24G8_TYPELESS;
        case GalFormat::D32_FLOAT:               return DXGI_FORMAT_D32_FLOAT;
        case GalFormat::D24_UNORM_S8_UINT:       return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default:                                 return DXGI_FORMAT_UNKNOWN;
    }
}

inline auto FromDxgiFormat(DXGI_FORMAT format) -> GalFormat {
    switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:         return GalFormat::R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:    return GalFormat::R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:      return GalFormat::R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:      return GalFormat::R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_FLOAT:         return GalFormat::R32G32B32_FLOAT;
        case DXGI_FORMAT_R32G32_FLOAT:            return GalFormat::R32G32_FLOAT;
        case DXGI_FORMAT_R11G11B10_FLOAT:         return GalFormat::R11G11B10_FLOAT;
        case DXGI_FORMAT_R32_FLOAT:               return GalFormat::R32_FLOAT;
        case DXGI_FORMAT_R16_FLOAT:               return GalFormat::R16_FLOAT;
        case DXGI_FORMAT_R32_TYPELESS:            return GalFormat::R32_TYPELESS;
        case DXGI_FORMAT_R24G8_TYPELESS:          return GalFormat::R24G8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:               return GalFormat::D32_FLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:       return GalFormat::D24_UNORM_S8_UINT;
        default:                                  return GalFormat::Unknown;
    }
}

inline auto ToD3DTopology(GalTopology topology) -> D3D11_PRIMITIVE_TOPOLOGY {
    switch (topology) {
        case GalTopology::TriangleList:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case GalTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case GalTopology::PatchList3:    return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        default:                         return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

inline auto FromD3DTopology(D3D11_PRIMITIVE_TOPOLOGY topology) -> GalTopology {
    switch (topology) {
        case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:              return GalTopology::TriangleList;
        case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:             return GalTopology::TriangleStrip;
        case D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST: return GalTopology::PatchList3;
        default:                                                 return GalTopology::Undefined;
    }
}

inline auto ToD3DBindFlags(GalBindFlags flags) -> uint32_t {
    uint32_t result = 0;
    if (HasFlag(flags, GalBindFlags::ShaderResource)) result |= D3D11_BIND_SHADER_RESOURCE;
    if (HasFlag(flags, GalBindFlags::RenderTarget))   result |= D3D11_BIND_RENDER_TARGET;
    if (HasFlag(flags, GalBindFlags::DepthStencil))   result |= D3D11_BIND_DEPTH_STENCIL;
    if (HasFlag(flags, GalBindFlags::ConstantBuffer))  result |= D3D11_BIND_CONSTANT_BUFFER;
    if (HasFlag(flags, GalBindFlags::VertexBuffer))    result |= D3D11_BIND_VERTEX_BUFFER;
    if (HasFlag(flags, GalBindFlags::IndexBuffer))     result |= D3D11_BIND_INDEX_BUFFER;
    return result;
}

inline auto FromD3DBindFlags(uint32_t flags) -> GalBindFlags {
    GalBindFlags result = GalBindFlags::None;
    if (flags & D3D11_BIND_SHADER_RESOURCE) result = result | GalBindFlags::ShaderResource;
    if (flags & D3D11_BIND_RENDER_TARGET)   result = result | GalBindFlags::RenderTarget;
    if (flags & D3D11_BIND_DEPTH_STENCIL)   result = result | GalBindFlags::DepthStencil;
    if (flags & D3D11_BIND_CONSTANT_BUFFER) result = result | GalBindFlags::ConstantBuffer;
    if (flags & D3D11_BIND_VERTEX_BUFFER)   result = result | GalBindFlags::VertexBuffer;
    if (flags & D3D11_BIND_INDEX_BUFFER)    result = result | GalBindFlags::IndexBuffer;
    return result;
}

} // namespace GalFormatConverter

#endif
