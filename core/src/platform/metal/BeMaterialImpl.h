#pragma once

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

struct BeMaterialImpl {
#ifdef __OBJC__
    id<MTLBuffer> cbuffer = nil;
#else
    void* cbuffer = nullptr;
#endif
    bool isFrequentlyUsed = false;
};
