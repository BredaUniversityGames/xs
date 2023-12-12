#include <device.h>
#include <log.h>
#include <configuration.h>
#include "fileio.h"
#include "device.h"
#include "device_apple.h"
#include "input.h"
#include "log.h"
#include "render.h"
#include "render_apple.h"
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

#elif defined(PLATFORM_MACOS)
#import <Cocoa/Cocoa.h>

@interface GameViewController : NSViewController
@end

#define IView NSView

#endif

using namespace std;


// Our platform independent renderer class. Implements the MTKViewDelegate protocol which
// allows it to accept per-frame update and drawable resize callbacks.
@interface XSRenderer : NSObject <MTKViewDelegate>

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;

@end

namespace xs::device::internal
{
    MTKView* _view;
    XSRenderer* _renderer;
    int _width = -1;
    int _height = -1;
    id<MTLCommandQueue> command_queue;      // The queue tied to the view
    id<MTLCommandBuffer> command_buffer;    // The buffer tied to the view
    id<MTLRenderCommandEncoder> render_encoder; // The encoder tied to the view
    auto prev_time = chrono::high_resolution_clock::now();
    
}
using namespace xs;
using namespace xs::device::internal;

@implementation XSRenderer {}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;
{
    self = [super init];
    if(self) {}
    return self;
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    auto current_time = chrono::high_resolution_clock::now();
    auto elapsed = current_time - prev_time;
    prev_time = current_time;
    auto dt = std::chrono::duration<double>(elapsed).count();
    if (dt > 0.03333)
    {
        log::warn("Running under than 30 fps. Time per frame:{}", dt);
        dt = 0.03333;
    }
        
    
    input::update(dt);
    // render::clear();
    script::update(dt);
    script::render();
    device::start_frame();
    render::render(); // render to texture
    device::internal::create_render_encoder();
    render::composite(); // composite into view
    inspector::render(float(dt));
    device::end_frame();
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    _width = size.width;
    _height = size.height;
}

@end

@implementation GameViewController {}

// Serves as an init point
- (void)viewDidLoad
{
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
    // audio::initialize();
    inspector::initialize();
    script::initialize();
}

@end

void device::initialize()
{
    command_queue = [_view.device newCommandQueue];
#if defined(PLATFORM_MACOS)
    CGFloat w = configuration::width();
    CGFloat h = configuration::height();
    //[_view setBoundsSize:{w, h}];
    //[_view setDrawableSize:{w, h}];
    [_view setFrameSize:{w, h}];
#endif
}

void device::shutdown()
{
    script::shutdown();
    inspector::shutdown();
    //audio::shutdown();
    input::shutdown();
    render::shutdown();
    data::shutdown();
    account::shutdown();
}

void device::start_frame()
{
    command_buffer = [command_queue commandBuffer];
    command_buffer.label = @"xs command buffer";
}

void device::end_frame()
{
    [render_encoder endEncoding];
    [command_buffer presentDrawable:_view.currentDrawable];
    [command_buffer commit];
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

MTKView* device::internal::get_view()
{
    return _view;
}

id<MTLCommandQueue> device::internal::get_command_queue()
{
    return command_queue;
}

id<MTLCommandBuffer> device::internal::get_command_buffer()
{
    return command_buffer;
}

id<MTLRenderCommandEncoder> device::internal::get_render_encoder()
{
    return render_encoder;
}

void device::internal::create_render_encoder()
{
    MTLRenderPassDescriptor* screen_rpd = _view.currentRenderPassDescriptor;
    render_encoder = [command_buffer renderCommandEncoderWithDescriptor:screen_rpd];
    render_encoder.label = @"xs screen render pass";
}
