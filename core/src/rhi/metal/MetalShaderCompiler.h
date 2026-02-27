#pragma once
#ifdef __APPLE__

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/access-modifiers.hpp>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

struct MetalShaderCompileResult {
    std::string MSLSource;
    std::string VertexEntryPoint;
    std::string FragmentEntryPoint;
};

class MetalShaderCompiler {

    expose
    static auto CompileHLSLtoMSL(
        const std::string& hlslSource,
        const std::string& vertexEntry,
        const std::string& vertexTarget,
        const std::string& pixelEntry,
        const std::string& pixelTarget
    ) -> std::expected<MetalShaderCompileResult, std::string>;

#ifdef __OBJC__
    static auto CreateLibrary(
        id<MTLDevice> device,
        const std::string& mslSource
    ) -> std::expected<id<MTLLibrary>, std::string>;
#endif

    hide
    static auto CompileHLSLtoSPIRV(
        const std::string& hlslSource,
        const std::string& entryPoint,
        const std::string& target
    ) -> std::expected<std::vector<uint32_t>, std::string>;

    static auto ConvertSPIRVtoMSL(
        const std::vector<uint32_t>& spirv
    ) -> std::expected<std::string, std::string>;
};

#endif
