#pragma once

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include "BeTypes.h"
#include <unordered_map>

namespace MetalUtils {

inline auto ToMTLPixelFormat(BeTextureFormat format) -> MTLPixelFormat {
    static const std::unordered_map<BeTextureFormat, MTLPixelFormat> map = {
        { BeTextureFormat::Unknown,            MTLPixelFormatInvalid },
        { BeTextureFormat::R8G8B8A8_UNorm,     MTLPixelFormatRGBA8Unorm },
        { BeTextureFormat::R11G11B10_Float,    MTLPixelFormatRG11B10Float },
        { BeTextureFormat::R16G16B16A16_Float, MTLPixelFormatRGBA16Float },
        { BeTextureFormat::R32_Typeless,       MTLPixelFormatR32Float },
        { BeTextureFormat::R24G8_Typeless,     MTLPixelFormatDepth32Float_Stencil8 },
        { BeTextureFormat::R16_Typeless,       MTLPixelFormatR16Float },
        { BeTextureFormat::R32_Float,          MTLPixelFormatR32Float },
        { BeTextureFormat::R16_UNorm,          MTLPixelFormatR16Unorm },
        { BeTextureFormat::R8_UNorm,           MTLPixelFormatR8Unorm },
    };
    return map.at(format);
}

inline auto ToDepthPixelFormat(BeTextureFormat format) -> MTLPixelFormat {
    static const std::unordered_map<BeTextureFormat, MTLPixelFormat> map = {
        { BeTextureFormat::R32_Typeless,   MTLPixelFormatDepth32Float },
        { BeTextureFormat::R24G8_Typeless, MTLPixelFormatDepth32Float_Stencil8 },
        { BeTextureFormat::R16_Typeless,   MTLPixelFormatDepth16Unorm },
    };
    return map.at(format);
}

inline auto ToShaderReadPixelFormat(BeTextureFormat format) -> MTLPixelFormat {
    static const std::unordered_map<BeTextureFormat, MTLPixelFormat> map = {
        { BeTextureFormat::R32_Typeless,   MTLPixelFormatR32Float },
        { BeTextureFormat::R24G8_Typeless, MTLPixelFormatR32Float },
        { BeTextureFormat::R16_Typeless,   MTLPixelFormatR16Float },
    };
    return map.at(format);
}

inline auto ToMTLPrimitiveType(BeTopology topology) -> MTLPrimitiveType {
    switch (topology) {
        case BeTopology::TriangleList:  return MTLPrimitiveTypeTriangle;
        case BeTopology::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
        default:                        return MTLPrimitiveTypeTriangle;
    }
}

inline auto HasDepthComponent(BeTextureFormat format) -> bool {
    return format == BeTextureFormat::R32_Typeless
        || format == BeTextureFormat::R24G8_Typeless
        || format == BeTextureFormat::R16_Typeless;
}

}

#endif
