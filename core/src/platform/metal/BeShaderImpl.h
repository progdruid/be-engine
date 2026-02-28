#pragma once

#include <string>
#include <vector>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

struct BeShaderVertexAttribute {
    std::string name;
    uint32_t offset;
    uint32_t format;
};

struct BeShaderImpl {
#ifdef __OBJC__
    id<MTLLibrary> vertexLibrary = nil;
    id<MTLFunction> vertexFunction = nil;
    id<MTLLibrary> pixelLibrary = nil;
    id<MTLFunction> pixelFunction = nil;
    id<MTLLibrary> hullLibrary = nil;
    id<MTLFunction> hullFunction = nil;
    id<MTLLibrary> domainLibrary = nil;
    id<MTLFunction> domainFunction = nil;
    MTLVertexDescriptor* vertexDescriptor = nil;
#else
    void* vertexLibrary = nullptr;
    void* vertexFunction = nullptr;
    void* pixelLibrary = nullptr;
    void* pixelFunction = nullptr;
    void* hullLibrary = nullptr;
    void* hullFunction = nullptr;
    void* domainLibrary = nullptr;
    void* domainFunction = nullptr;
    void* vertexDescriptor = nullptr;
#endif
    std::vector<BeShaderVertexAttribute> vertexAttributes;
};
