#pragma once
#ifdef __APPLE__

#include "../RhiTypes.h"

#ifdef __OBJC__
#import <Metal/Metal.h>

namespace MetalFormatConverter {

inline auto ToMTLPixelFormat(RhiFormat format) -> MTLPixelFormat {
    switch (format) {
        case RhiFormat::R8G8B8A8_UNORM:         return MTLPixelFormatRGBA8Unorm;
        case RhiFormat::R8G8B8A8_UNORM_SRGB:    return MTLPixelFormatRGBA8Unorm_sRGB;
        case RhiFormat::R16G16B16A16_FLOAT:      return MTLPixelFormatRGBA16Float;
        case RhiFormat::R32G32B32A32_FLOAT:      return MTLPixelFormatRGBA32Float;
        case RhiFormat::R32G32_FLOAT:            return MTLPixelFormatRG32Float;
        case RhiFormat::R11G11B10_FLOAT:         return MTLPixelFormatRG11B10Float;
        case RhiFormat::R32_FLOAT:               return MTLPixelFormatR32Float;
        case RhiFormat::R16_FLOAT:               return MTLPixelFormatR16Float;
        case RhiFormat::D32_FLOAT:               return MTLPixelFormatDepth32Float;
        case RhiFormat::D24_UNORM_S8_UINT:       return MTLPixelFormatDepth24Unorm_Stencil8;
        case RhiFormat::R32_TYPELESS:            return MTLPixelFormatR32Float;
        case RhiFormat::R24G8_TYPELESS:          return MTLPixelFormatDepth24Unorm_Stencil8;
        default:                                 return MTLPixelFormatInvalid;
    }
}

inline auto FromMTLPixelFormat(MTLPixelFormat format) -> RhiFormat {
    switch (format) {
        case MTLPixelFormatRGBA8Unorm:           return RhiFormat::R8G8B8A8_UNORM;
        case MTLPixelFormatRGBA8Unorm_sRGB:      return RhiFormat::R8G8B8A8_UNORM_SRGB;
        case MTLPixelFormatRGBA16Float:           return RhiFormat::R16G16B16A16_FLOAT;
        case MTLPixelFormatRGBA32Float:           return RhiFormat::R32G32B32A32_FLOAT;
        case MTLPixelFormatRG32Float:             return RhiFormat::R32G32_FLOAT;
        case MTLPixelFormatRG11B10Float:          return RhiFormat::R11G11B10_FLOAT;
        case MTLPixelFormatR32Float:              return RhiFormat::R32_FLOAT;
        case MTLPixelFormatR16Float:              return RhiFormat::R16_FLOAT;
        case MTLPixelFormatDepth32Float:          return RhiFormat::D32_FLOAT;
        case MTLPixelFormatDepth24Unorm_Stencil8: return RhiFormat::D24_UNORM_S8_UINT;
        default:                                  return RhiFormat::Unknown;
    }
}

inline auto ToMTLPrimitiveType(RhiTopology topology) -> MTLPrimitiveType {
    switch (topology) {
        case RhiTopology::TriangleList:  return MTLPrimitiveTypeTriangle;
        case RhiTopology::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
        default:                         return MTLPrimitiveTypeTriangle;
    }
}

} // namespace MetalFormatConverter

#endif
#endif
