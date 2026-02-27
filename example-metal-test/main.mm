#ifdef __APPLE__

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cmath>
#include <iostream>
#include <umbrellas/include-glm.h>

static const char* shaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float4 color    [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float4 color;
    float3 worldPos;
};

struct Uniforms {
    float4x4 projectionView;
    float4x4 model;
    float3 lightDir;
    float3 lightColor;
    float3 ambientColor;
    float time;
};

vertex VertexOut vertexMain(
    VertexIn in [[stage_in]],
    constant Uniforms& uniforms [[buffer(1)]]
) {
    VertexOut out;
    float4 worldPos = uniforms.model * float4(in.position, 1.0);
    out.position = uniforms.projectionView * worldPos;
    out.normal = normalize((uniforms.model * float4(in.normal, 0.0)).xyz);
    out.color = in.color;
    out.worldPos = worldPos.xyz;
    return out;
}

fragment float4 fragmentMain(
    VertexOut in [[stage_in]],
    constant Uniforms& uniforms [[buffer(1)]]
) {
    float3 N = normalize(in.normal);
    float3 L = normalize(-uniforms.lightDir);
    float NdotL = max(dot(N, L), 0.0);
    
    float3 diffuse = uniforms.lightColor * NdotL;
    float3 ambient = uniforms.ambientColor;
    float3 finalColor = in.color.rgb * (diffuse + ambient);
    
    return float4(finalColor, 1.0);
}
)";

struct Uniforms {
    glm::mat4 projectionView;
    glm::mat4 model;
    glm::vec3 lightDir;
    float _pad0;
    glm::vec3 lightColor;
    float _pad1;
    glm::vec3 ambientColor;
    float time;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
};

static std::vector<Vertex> makeCubeVertices() {
    std::vector<Vertex> verts;
    
    const glm::vec3 faces[6][4] = {
        {{-1,-1, 1},{ 1,-1, 1},{ 1, 1, 1},{-1, 1, 1}},
        {{ 1,-1,-1},{-1,-1,-1},{-1, 1,-1},{ 1, 1,-1}},
        {{-1, 1, 1},{ 1, 1, 1},{ 1, 1,-1},{-1, 1,-1}},
        {{-1,-1,-1},{ 1,-1,-1},{ 1,-1, 1},{-1,-1, 1}},
        {{ 1,-1, 1},{ 1,-1,-1},{ 1, 1,-1},{ 1, 1, 1}},
        {{-1,-1,-1},{-1,-1, 1},{-1, 1, 1},{-1, 1,-1}},
    };
    const glm::vec3 normals[6] = {
        { 0, 0, 1},{ 0, 0,-1},{ 0, 1, 0},{ 0,-1, 0},{ 1, 0, 0},{-1, 0, 0}
    };
    const glm::vec4 colors[6] = {
        {1.0, 0.3, 0.3, 1}, {0.3, 1.0, 0.3, 1}, {0.3, 0.3, 1.0, 1},
        {1.0, 1.0, 0.3, 1}, {1.0, 0.3, 1.0, 1}, {0.3, 1.0, 1.0, 1}
    };
    
    for (int f = 0; f < 6; ++f) {
        auto n = normals[f];
        auto c = colors[f];
        verts.push_back({faces[f][0], n, c});
        verts.push_back({faces[f][1], n, c});
        verts.push_back({faces[f][2], n, c});
        verts.push_back({faces[f][0], n, c});
        verts.push_back({faces[f][2], n, c});
        verts.push_back({faces[f][3], n, c});
    }
    return verts;
}

int main() {
    @autoreleasepool {
        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";
            return 1;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(1280, 720, "be-engine: Metal Test", nullptr, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";
            glfwTerminate();
            return 1;
        }

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            std::cerr << "No Metal device\n";
            return 1;
        }
        std::cout << "Metal device: " << [[device name] UTF8String] << "\n";

        NSWindow* nswindow = glfwGetCocoaWindow(window);
        NSView* view = [nswindow contentView];

        CAMetalLayer* metalLayer = [CAMetalLayer layer];
        metalLayer.device = device;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
        metalLayer.framebufferOnly = NO;
        [view setWantsLayer:YES];
        [view setLayer:metalLayer];

        id<MTLCommandQueue> commandQueue = [device newCommandQueue];

        NSError* error = nil;
        id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:shaderSource]
                                                      options:nil
                                                        error:&error];
        if (error) {
            std::cerr << "Shader compile error: " << [[error localizedDescription] UTF8String] << "\n";
            return 1;
        }

        id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertexMain"];
        id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"fragmentMain"];

        MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
        vertexDesc.attributes[0].format = MTLVertexFormatFloat3;
        vertexDesc.attributes[0].offset = 0;
        vertexDesc.attributes[0].bufferIndex = 0;
        vertexDesc.attributes[1].format = MTLVertexFormatFloat3;
        vertexDesc.attributes[1].offset = sizeof(glm::vec3);
        vertexDesc.attributes[1].bufferIndex = 0;
        vertexDesc.attributes[2].format = MTLVertexFormatFloat4;
        vertexDesc.attributes[2].offset = sizeof(glm::vec3) * 2;
        vertexDesc.attributes[2].bufferIndex = 0;
        vertexDesc.layouts[0].stride = sizeof(Vertex);

        MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDesc.vertexFunction = vertexFunc;
        pipelineDesc.fragmentFunction = fragmentFunc;
        pipelineDesc.vertexDescriptor = vertexDesc;
        pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
        pipelineDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
        if (error) {
            std::cerr << "Pipeline error: " << [[error localizedDescription] UTF8String] << "\n";
            return 1;
        }

        MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthDesc.depthCompareFunction = MTLCompareFunctionLess;
        depthDesc.depthWriteEnabled = YES;
        id<MTLDepthStencilState> depthState = [device newDepthStencilStateWithDescriptor:depthDesc];

        auto cubeVerts = makeCubeVertices();
        id<MTLBuffer> vertexBuffer = [device newBufferWithBytes:cubeVerts.data()
                                                         length:cubeVerts.size() * sizeof(Vertex)
                                                        options:MTLResourceStorageModeShared];

        id<MTLBuffer> uniformBuffer = [device newBufferWithLength:sizeof(Uniforms)
                                                          options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];

        MTLTextureDescriptor* depthTexDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                               width:1280
                                                                                              height:720
                                                                                           mipmapped:NO];
        depthTexDesc.usage = MTLTextureUsageRenderTarget;
        depthTexDesc.storageMode = MTLStorageModePrivate;
        id<MTLTexture> depthTexture = [device newTextureWithDescriptor:depthTexDesc];

        std::cout << "be-engine Metal test running. Close window to exit.\n";

        while (!glfwWindowShouldClose(window)) {
            @autoreleasepool {
                glfwPollEvents();

                int w, h;
                glfwGetFramebufferSize(window, &w, &h);
                metalLayer.drawableSize = CGSizeMake(w, h);

                if (depthTexture.width != (NSUInteger)w || depthTexture.height != (NSUInteger)h) {
                    depthTexDesc.width = w;
                    depthTexDesc.height = h;
                    depthTexture = [device newTextureWithDescriptor:depthTexDesc];
                }

                double time = glfwGetTime();

                float aspect = (float)w / (float)h;
                glm::mat4 proj = glm::perspectiveLH_ZO(glm::radians(60.0f), aspect, 0.1f, 100.0f);
                glm::mat4 view = glm::lookAtLH(
                    glm::vec3(3.0f * cos(time * 0.5f), 2.0f, 3.0f * sin(time * 0.5f)),
                    glm::vec3(0, 0, 0),
                    glm::vec3(0, 1, 0)
                );

                Uniforms uniforms;
                uniforms.projectionView = proj * view;
                uniforms.model = glm::rotate(glm::mat4(1.0f), (float)time * 0.7f, glm::vec3(0, 1, 0));
                uniforms.lightDir = glm::normalize(glm::vec3(-0.8f, -1.0f, -0.8f));
                uniforms.lightColor = glm::vec3(0.9f, 0.85f, 0.8f);
                uniforms.ambientColor = glm::vec3(0.15f);
                uniforms.time = (float)time;

                memcpy([uniformBuffer contents], &uniforms, sizeof(Uniforms));

                id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
                if (!drawable) continue;

                MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
                passDesc.colorAttachments[0].texture = drawable.texture;
                passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
                passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
                passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.02, 0.02, 0.05, 1.0);
                passDesc.depthAttachment.texture = depthTexture;
                passDesc.depthAttachment.loadAction = MTLLoadActionClear;
                passDesc.depthAttachment.storeAction = MTLStoreActionDontCare;
                passDesc.depthAttachment.clearDepth = 1.0;

                id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
                id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];

                [encoder setRenderPipelineState:pipelineState];
                [encoder setDepthStencilState:depthState];
                [encoder setCullMode:MTLCullModeBack];
                [encoder setFrontFacingWinding:MTLWindingCounterClockwise];

                MTLViewport viewport = {0, 0, (double)w, (double)h, 0, 1};
                [encoder setViewport:viewport];

                [encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
                [encoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
                [encoder setFragmentBuffer:uniformBuffer offset:0 atIndex:1];

                [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:cubeVerts.size()];

                [encoder endEncoding];
                [commandBuffer presentDrawable:drawable];
                [commandBuffer commit];
            }
        }

        glfwDestroyWindow(window);
        glfwTerminate();
    }
    return 0;
}

#endif
