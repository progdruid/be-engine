#pragma once

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

struct BeBRPSubmissionBufferImpl {
#ifdef __OBJC__
    id<MTLDevice> device = nil;
    id<MTLBuffer> sharedVertexBuffer = nil;
    id<MTLBuffer> sharedIndexBuffer = nil;
#else
    void* device = nullptr;
    void* sharedVertexBuffer = nullptr;
    void* sharedIndexBuffer = nullptr;
#endif
};
