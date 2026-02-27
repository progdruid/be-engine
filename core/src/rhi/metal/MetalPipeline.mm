#ifdef __APPLE__

#import "MetalPipeline.h"
#import "MetalFormatConverter.h"
#import <Metal/Metal.h>

auto MetalPipeline::Create(
    id<MTLDevice> device,
    id<MTLLibrary> library,
    const MetalPipelineDesc& desc
) -> std::shared_ptr<MetalPipeline> {

    auto pipeline = std::shared_ptr<MetalPipeline>(new MetalPipeline());
    pipeline->_library = library;

    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];

    id<MTLFunction> vertexFunc = [library newFunctionWithName:[NSString stringWithUTF8String:desc.VertexFunction.c_str()]];
    id<MTLFunction> fragmentFunc = [library newFunctionWithName:[NSString stringWithUTF8String:desc.FragmentFunction.c_str()]];

    pipelineDesc.vertexFunction = vertexFunc;
    pipelineDesc.fragmentFunction = fragmentFunc;

    for (uint32_t i = 0; i < desc.ColorFormatCount; ++i) {
        pipelineDesc.colorAttachments[i].pixelFormat = MetalFormatConverter::ToMTLPixelFormat(desc.ColorFormats[i]);

        if (desc.BlendEnabled) {
            pipelineDesc.colorAttachments[i].blendingEnabled = YES;
            pipelineDesc.colorAttachments[i].sourceRGBBlendFactor = MTLBlendFactorOne;
            pipelineDesc.colorAttachments[i].destinationRGBBlendFactor = MTLBlendFactorOne;
            pipelineDesc.colorAttachments[i].rgbBlendOperation = MTLBlendOperationAdd;
            pipelineDesc.colorAttachments[i].sourceAlphaBlendFactor = MTLBlendFactorOne;
            pipelineDesc.colorAttachments[i].destinationAlphaBlendFactor = MTLBlendFactorOne;
            pipelineDesc.colorAttachments[i].alphaBlendOperation = MTLBlendOperationAdd;
        }
    }

    if (desc.DepthFormat != RhiFormat::Unknown) {
        pipelineDesc.depthAttachmentPixelFormat = MetalFormatConverter::ToMTLPixelFormat(desc.DepthFormat);
    }

    NSError* error = nil;
    pipeline->_pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (error) {
        NSLog(@"Metal pipeline creation failed: %@", error);
        return nullptr;
    }

    MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthDesc.depthWriteEnabled = YES;
    pipeline->_depthStencilState = [device newDepthStencilStateWithDescriptor:depthDesc];

    return pipeline;
}

MetalPipeline::~MetalPipeline() {
    _pipelineState = nil;
    _depthStencilState = nil;
    _library = nil;
}

#endif
