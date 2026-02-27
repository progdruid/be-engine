#pragma once
#include "RhiTypes.h"

#ifdef _WIN32
#include <d3d11.h>

namespace RhiFormatConverter {

inline auto ToDxgiFormat(RhiFormat format) -> DXGI_FORMAT {
    switch (format) {
        case RhiFormat::R8G8B8A8_UNORM:         return DXGI_FORMAT_R8G8B8A8_UNORM;
        case RhiFormat::R8G8B8A8_UNORM_SRGB:    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case RhiFormat::R16G16B16A16_FLOAT:      return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case RhiFormat::R32G32B32A32_FLOAT:      return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case RhiFormat::R32G32B32_FLOAT:         return DXGI_FORMAT_R32G32B32_FLOAT;
        case RhiFormat::R32G32_FLOAT:            return DXGI_FORMAT_R32G32_FLOAT;
        case RhiFormat::R11G11B10_FLOAT:         return DXGI_FORMAT_R11G11B10_FLOAT;
        case RhiFormat::R32_FLOAT:               return DXGI_FORMAT_R32_FLOAT;
        case RhiFormat::R16_FLOAT:               return DXGI_FORMAT_R16_FLOAT;
        case RhiFormat::R32_TYPELESS:            return DXGI_FORMAT_R32_TYPELESS;
        case RhiFormat::R24G8_TYPELESS:          return DXGI_FORMAT_R24G8_TYPELESS;
        case RhiFormat::D32_FLOAT:               return DXGI_FORMAT_D32_FLOAT;
        case RhiFormat::D24_UNORM_S8_UINT:       return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default:                                 return DXGI_FORMAT_UNKNOWN;
    }
}

inline auto FromDxgiFormat(DXGI_FORMAT format) -> RhiFormat {
    switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:         return RhiFormat::R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:    return RhiFormat::R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:      return RhiFormat::R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:      return RhiFormat::R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_FLOAT:         return RhiFormat::R32G32B32_FLOAT;
        case DXGI_FORMAT_R32G32_FLOAT:            return RhiFormat::R32G32_FLOAT;
        case DXGI_FORMAT_R11G11B10_FLOAT:         return RhiFormat::R11G11B10_FLOAT;
        case DXGI_FORMAT_R32_FLOAT:               return RhiFormat::R32_FLOAT;
        case DXGI_FORMAT_R16_FLOAT:               return RhiFormat::R16_FLOAT;
        case DXGI_FORMAT_R32_TYPELESS:            return RhiFormat::R32_TYPELESS;
        case DXGI_FORMAT_R24G8_TYPELESS:          return RhiFormat::R24G8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:               return RhiFormat::D32_FLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:       return RhiFormat::D24_UNORM_S8_UINT;
        default:                                  return RhiFormat::Unknown;
    }
}

inline auto ToD3DTopology(RhiTopology topology) -> D3D11_PRIMITIVE_TOPOLOGY {
    switch (topology) {
        case RhiTopology::TriangleList:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case RhiTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case RhiTopology::PatchList3:    return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        default:                         return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

inline auto FromD3DTopology(D3D11_PRIMITIVE_TOPOLOGY topology) -> RhiTopology {
    switch (topology) {
        case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:              return RhiTopology::TriangleList;
        case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:             return RhiTopology::TriangleStrip;
        case D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST: return RhiTopology::PatchList3;
        default:                                                 return RhiTopology::Undefined;
    }
}

inline auto ToD3DBindFlags(RhiBindFlags flags) -> uint32_t {
    uint32_t result = 0;
    if (HasFlag(flags, RhiBindFlags::ShaderResource)) result |= D3D11_BIND_SHADER_RESOURCE;
    if (HasFlag(flags, RhiBindFlags::RenderTarget))   result |= D3D11_BIND_RENDER_TARGET;
    if (HasFlag(flags, RhiBindFlags::DepthStencil))   result |= D3D11_BIND_DEPTH_STENCIL;
    if (HasFlag(flags, RhiBindFlags::ConstantBuffer))  result |= D3D11_BIND_CONSTANT_BUFFER;
    if (HasFlag(flags, RhiBindFlags::VertexBuffer))    result |= D3D11_BIND_VERTEX_BUFFER;
    if (HasFlag(flags, RhiBindFlags::IndexBuffer))     result |= D3D11_BIND_INDEX_BUFFER;
    return result;
}

inline auto FromD3DBindFlags(uint32_t flags) -> RhiBindFlags {
    RhiBindFlags result = RhiBindFlags::None;
    if (flags & D3D11_BIND_SHADER_RESOURCE) result = result | RhiBindFlags::ShaderResource;
    if (flags & D3D11_BIND_RENDER_TARGET)   result = result | RhiBindFlags::RenderTarget;
    if (flags & D3D11_BIND_DEPTH_STENCIL)   result = result | RhiBindFlags::DepthStencil;
    if (flags & D3D11_BIND_CONSTANT_BUFFER) result = result | RhiBindFlags::ConstantBuffer;
    if (flags & D3D11_BIND_VERTEX_BUFFER)   result = result | RhiBindFlags::VertexBuffer;
    if (flags & D3D11_BIND_INDEX_BUFFER)    result = result | RhiBindFlags::IndexBuffer;
    return result;
}

} // namespace RhiFormatConverter

#endif
