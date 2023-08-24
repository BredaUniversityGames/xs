#include <device.h>
#include <log.h>
#include <configuration.h>
#include "fileio.h"
#include "device.h"
#include "device_apple.h"
#include "input.h"
#include "log.h"
#include "render.h"
#include "script.h"
#include "audio.h"
#include "account.h"
#include "data.h"
#include "inspector.h"
#import <MetalKit/MetalKit.h>

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>

// Our iOS view controller
@interface GameViewController : UIViewController
@end

#define IView UIView

#elif defined(PLATFORM_APPLE_MACOS)
#import <Cocoa/Cocoa.h>

@interface GameViewController : NSViewController
@end

#define IView NSView

#endif

using namespace std;


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
    // auto prev_time = chrono::high_resolution_clock::now();
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
    
    render::render();
    
    // auto current_time = chrono::high_resolution_clock::now();
    // auto elapsed = current_time - prev_time;
    // prev_time = current_time;
    // auto dt = std::chrono::duration<double>(elapsed).count();
    // if (dt > 0.03333) dt = 0.03333;

    auto dt = 0.0666;
    device::poll_events();
    input::update(dt);
    render::clear();
    script::update(dt);
    script::render();
    render::render();
    // inspector::render(float(dt));
    device::swap_buffers();
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
    // Initialize everything
    
    [super viewDidLoad];

    _view = (MTKView *)self.view;

    _view.device = MTLCreateSystemDefaultDevice();

    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[IView alloc] initWithFrame:self.view.frame];
        return;
    }

    _renderer = [[XSRenderer alloc] initWithMetalKitView:_view];

    [_renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];

    _view.delegate = _renderer;
    
    log::initialize();
    account::initialize();
    fileio::initialize();
    data::initialize();
    script::configure();
    device::initialize();
    render::initialize();
    input::initialize();
    //audio::initialize();
    inspector::initialize();
    script::initialize();
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
    return _width;
}

int device::get_height()
{
    return _height;
}

MTKView* device::get_view()
{
    return _view;
}
