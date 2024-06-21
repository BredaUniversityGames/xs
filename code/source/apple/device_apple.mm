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
typedef  CGPoint XsPoint;

// Our iOS view controller
@interface GameViewController : UIViewController
@end

#define IView UIView

#elif defined(PLATFORM_MAC)
#import <Cocoa/Cocoa.h>
typedef  NSPoint XsPoint;

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
    float _scaling = 1.0f;
    float _hdpi_scale = 1.0f;
    
    id<MTLCommandQueue> command_queue;      // The queue tied to the view
    id<MTLCommandBuffer> command_buffer;    // The buffer tied to the view
    id<MTLRenderCommandEncoder> render_encoder; // The encoder tied to the view
    auto prev_time = chrono::high_resolution_clock::now();
}
namespace xs::input
{
    XsPoint mouse_pos = {0, 0};
    bool clicked = false;
    bool clicked_last_frame = false;
}

using namespace xs;
using namespace xs::device::internal;
using namespace xs::input;

@implementation XSRenderer {}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;
{
    self = [super init];
    if(self) {}
    return self;
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
#if defined(PLATFORM_MAC)
     _view.window.acceptsMouseMovedEvents = true;
#endif
    
    auto current_time = chrono::high_resolution_clock::now();
    auto elapsed = current_time - prev_time;
    prev_time = current_time;
    auto dt = std::chrono::duration<double>(elapsed).count();
    if (dt > 0.03333)
        dt = 0.03333;
    input::update(dt);
    if (!inspector::paused())
    {
        render::clear();
        script::update(dt);
        //audio::update(dt);
        script::render();
    }
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
    _scaling = _width / (configuration::width() * configuration::multiplier());
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

#if defined(PLATFORM_MAC)

- (void)mouseUp:(NSEvent *)event
{
    input::mouse_pos = [event locationInWindow];
    input::clicked = false;
}

- (void)mouseDown:(NSEvent *)event
{
    input::mouse_pos = [event locationInWindow];
    input::clicked = true;
}

- (void)mouseMoved:(NSEvent*) event
{
    input::mouse_pos = [event locationInWindow];
}

- (void)mouseDragged:(NSEvent *)event
{
    input::mouse_pos = [event locationInWindow];
}


#elif TARGET_OS_IOS

-(bool)prefersHomeIndicatorAutoHidden{
    return YES;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        mouse_pos.x = [t locationInView:t.view].x;
        mouse_pos.y = configuration::height() - [t locationInView:t.view].y;
    }
    input::clicked = true;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        mouse_pos.x = [t locationInView:t.view].x;
        mouse_pos.y = configuration::height() - [t locationInView:t.view].y;
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        mouse_pos.x = [t locationInView:t.view].x;
        mouse_pos.y = configuration::height() - [t locationInView:t.view].y;
    }
    input::clicked = false;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches)
    {
        mouse_pos.x = -1000000.0f;
        mouse_pos.y = -1000000.0f;
    }
    input::clicked = false;
}

#endif

@end

void device::initialize()
{
    command_queue = [_view.device newCommandQueue];
#if defined(PLATFORM_MAC)
    CGFloat w = configuration::width() * configuration::multiplier();
    CGFloat h = configuration::height() * configuration::multiplier();;
    auto hdpi_scale = [[NSScreen mainScreen] backingScaleFactor];
    if(xs::configuration::window_size_in_points())
        [_view setFrameSize:{w, h}];    // In pixels
    else
        [_view setFrameSize:{w / hdpi_scale, h / hdpi_scale}]; // In points
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
    input::clicked_last_frame = input::clicked;
    
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

float device::hdpi_scaling()
{
    return _hdpi_scale;
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

double xs::input::get_mouse_x()
{
    float m = configuration::multiplier();
    float s = _scaling;
    return (input::mouse_pos.x / m) - (_width / (m * s * 2.0f));
}

double xs::input::get_mouse_y()
{
    float m = configuration::multiplier();
    float s = _scaling;
    return (input::mouse_pos.y / m) - (_height / (m * s * 2.0f));
}

bool xs::input::get_mousebutton(mouse_button button)
{
    return input::clicked;
}

bool xs::input::get_mousebutton_once(mouse_button button)
{
    return input::clicked && !input::clicked_last_frame;
}

