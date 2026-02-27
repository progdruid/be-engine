#ifdef __APPLE__

#import "MetalSwapchain.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

MetalSwapchain::MetalSwapchain(id<MTLDevice> device, void* nativeWindowLayer, uint32_t width, uint32_t height)
    : _device(device)
    , _currentDrawable(nil)
    , _width(width)
    , _height(height)
{
    _layer = [CAMetalLayer layer];
    _layer.device = device;
    _layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _layer.drawableSize = CGSizeMake(width, height);
    _layer.framebufferOnly = NO;

    NSView* view = (__bridge NSView*)nativeWindowLayer;
    [view setWantsLayer:YES];
    [view setLayer:_layer];
}

MetalSwapchain::~MetalSwapchain() {
    _currentDrawable = nil;
    _layer = nil;
}

auto MetalSwapchain::Present(uint32_t syncInterval) -> void {
    if (_currentDrawable) {
        [_currentDrawable present];
        _currentDrawable = nil;
    }
}

auto MetalSwapchain::Resize(uint32_t width, uint32_t height) -> void {
    _width = width;
    _height = height;
    _layer.drawableSize = CGSizeMake(width, height);
}

auto MetalSwapchain::GetCurrentDrawable() -> id<CAMetalDrawable> {
    if (!_currentDrawable) {
        _currentDrawable = [_layer nextDrawable];
    }
    return _currentDrawable;
}

#endif
