#ifdef __APPLE__

#import "MetalShaderCompiler.h"
#import <Metal/Metal.h>

// NOTE: This implementation requires SPIRV-Cross and DXC (DirectXShaderCompiler)
// to be available as libraries. For the initial version, we provide the structure
// and a fallback that loads pre-compiled MSL from .metal files.
//
// Build dependencies:
//   brew install spirv-cross
//   brew install directxshadercompiler
//
// The full pipeline: HLSL → DXC → SPIR-V → SPIRV-Cross → MSL → MTLLibrary
//
// For vibe-coding the initial port, the recommended approach is:
// 1. Use SPIRV-Cross CLI to batch-convert all .hlsl shaders to .metal files
// 2. Load .metal files directly in the engine
// 3. Later, integrate SPIRV-Cross as a library for runtime compilation

auto MetalShaderCompiler::CompileHLSLtoSPIRV(
    const std::string& hlslSource,
    const std::string& entryPoint,
    const std::string& target
) -> std::expected<std::vector<uint32_t>, std::string> {

    // TODO: Integrate DXC library for runtime HLSL → SPIR-V
    // For now, this should be done offline:
    //   dxc -spirv -T vs_6_0 -E VSMain shader.hlsl -Fo shader.vert.spv
    //   dxc -spirv -T ps_6_0 -E PSMain shader.hlsl -Fo shader.frag.spv

    return std::unexpected("Runtime HLSL→SPIR-V not yet integrated. Use offline compilation.");
}

auto MetalShaderCompiler::ConvertSPIRVtoMSL(
    const std::vector<uint32_t>& spirv
) -> std::expected<std::string, std::string> {

    // TODO: Integrate SPIRV-Cross library for runtime SPIR-V → MSL
    // For now, this should be done offline:
    //   spirv-cross --msl shader.vert.spv --output shader.vert.metal
    //   spirv-cross --msl shader.frag.spv --output shader.frag.metal

    return std::unexpected("Runtime SPIR-V→MSL not yet integrated. Use offline compilation.");
}

auto MetalShaderCompiler::CompileHLSLtoMSL(
    const std::string& hlslSource,
    const std::string& vertexEntry,
    const std::string& vertexTarget,
    const std::string& pixelEntry,
    const std::string& pixelTarget
) -> std::expected<MetalShaderCompileResult, std::string> {

    auto vertexSPIRV = CompileHLSLtoSPIRV(hlslSource, vertexEntry, vertexTarget);
    if (!vertexSPIRV) return std::unexpected(vertexSPIRV.error());

    auto pixelSPIRV = CompileHLSLtoSPIRV(hlslSource, pixelEntry, pixelTarget);
    if (!pixelSPIRV) return std::unexpected(pixelSPIRV.error());

    auto vertexMSL = ConvertSPIRVtoMSL(vertexSPIRV.value());
    if (!vertexMSL) return std::unexpected(vertexMSL.error());

    auto pixelMSL = ConvertSPIRVtoMSL(pixelSPIRV.value());
    if (!pixelMSL) return std::unexpected(pixelMSL.error());

    MetalShaderCompileResult result;
    result.MSLSource = vertexMSL.value() + "\n" + pixelMSL.value();
    result.VertexEntryPoint = vertexEntry + "0";
    result.FragmentEntryPoint = pixelEntry + "0";

    return result;
}

auto MetalShaderCompiler::CreateLibrary(
    id<MTLDevice> device,
    const std::string& mslSource
) -> std::expected<id<MTLLibrary>, std::string> {

    NSString* source = [NSString stringWithUTF8String:mslSource.c_str()];
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    options.languageVersion = MTLLanguageVersion3_0;

    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:source options:options error:&error];

    if (error) {
        return std::unexpected(std::string([[error localizedDescription] UTF8String]));
    }

    return library;
}

#endif
