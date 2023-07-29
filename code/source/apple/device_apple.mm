#include <device.h>
#include <log.h>
#include <opengl.h>
#include <configuration.h>

#import <Cocoa/Cocoa.h>

#import "GameViewController.h"
//#import "Renderer.h"

#import <MetalKit/MetalKit.h>

// Our platform independent renderer class.   Implements the MTKViewDelegate protocol which
//   allows it to accept per-frame update and drawable resize callbacks.
@interface XSRenderer : NSObject <MTKViewDelegate>

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;

@end

namespace xs::device::internal
{
    MTKView* _view;
    XSRenderer* _renderer;
    int _width = -1;
    int _height = -1;
}
using namespace xs;
using namespace xs::device::internal;

@implementation XSRenderer
{
}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;
{
    self = [super init];
    if(self)
    {
    }

    return self;
}


- (void)drawInMTKView:(nonnull MTKView *)view
{
    int g = 0;
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    _width = size.width;
    _height = size.height;
    float aspect = size.width / (float)size.height;
}
@end

@implementation GameViewController {}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _view = (MTKView *)self.view;

    _view.device = MTLCreateSystemDefaultDevice();

    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[NSView alloc] initWithFrame:self.view.frame];
        return;
    }

    _renderer = [[XSRenderer alloc] initWithMetalKitView:_view];

    [_renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];

    _view.delegate = _renderer;
}

@end

void device::initialize()
{
}

void device::shutdown()
{
}

void device::swap_buffers()
{
}

void device::poll_events()
{
}

bool device::can_close()
{
    return false;
}

bool device::request_close()
{
    return false;
}

bool device::should_close()
{
    return false;
}

int device::get_width()
{
    return 0;
}

int device::get_height()
{
    return 0;
}
