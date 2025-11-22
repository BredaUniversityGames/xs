#include "device.hpp"
#include "device_apple.h"
#include "render_apple.h"
#include "log.hpp"
#include "configuration.hpp"
#include "fileio.hpp"
#include "input.hpp"
#include "render.hpp"
#include "script.hpp"
#include "audio.hpp"
#include "account.hpp"
#include "data.hpp"
#include "inspector.hpp"
#include "profiler.hpp"
#include <SDL.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#ifdef INSPECTOR
#include <imgui.h>
#include <imgui_impl.h>
#endif


using namespace std;
using namespace xs;
using namespace xs::device;

namespace xs::device::internal
{
    SDL_Window* window = nullptr;
    SDL_MetalView metal_view = nullptr;
    int    width = -1;
    int    height = -1;
    bool quit = false;

    // Metal objects
    id<MTLDevice> metal_device = nil;
    id<MTLCommandQueue> command_queue = nil;
    id<MTLCommandBuffer> command_buffer = nil;
    id<MTLRenderCommandEncoder> render_encoder = nil;
    id<CAMetalDrawable> current_drawable = nil;
}

void device::initialize()
{
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        log::critical("SDL init failed: {}", SDL_GetError());
        assert(false);
        exit(EXIT_FAILURE);
    }

    log::info("SDL version {}.{}.{}", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

    // Set window size
    internal::width = configuration::width() * configuration::multiplier();
    internal::height = configuration::height() * configuration::multiplier();

    // Create window with Metal support
    internal::window = SDL_CreateWindow(
        configuration::title().c_str(),
        internal::width,
        internal::height,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!internal::window)
    {
        log::critical("SDL window could not be created: {}", SDL_GetError());
        SDL_Quit();
        assert(false);
        exit(EXIT_FAILURE);
    }

    SDL_ShowWindow(internal::window);

    // Initialize Metal
    internal::metal_view = SDL_Metal_CreateView(internal::window);
    if (!internal::metal_view)
    {
        log::critical("Failed to create Metal view: {}", SDL_GetError());
        SDL_DestroyWindow(internal::window);
        SDL_Quit();
        assert(false);
        exit(EXIT_FAILURE);
    }

    // Get the Metal layer and create MTKView
    CAMetalLayer* metal_layer = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(internal::metal_view);
    internal::metal_device = metal_layer.device;

    if (!internal::metal_device)
    {
        internal::metal_device = MTLCreateSystemDefaultDevice();
        metal_layer.device = internal::metal_device;
    }

    // Create command queue
    internal::command_queue = [internal::metal_device newCommandQueue];

    // Create MTKView wrapper for compatibility with existing render code
    // Note: We're using the Metal layer from SDL, not a real MTKView
    // The render code will need to be adapted to work with this

    log::info("SDL3 window created with Metal support");
}

void device::shutdown()
{
    if (internal::window)
    {
        SDL_DestroyWindow(internal::window);
        internal::window = nullptr;
    }
    SDL_Quit();
}

void device::begin_frame()
{
    // Create the command buffer for this frame
    internal::command_buffer = [internal::command_queue commandBuffer];
    internal::command_buffer.label = @"xs frame command buffer";
    
    // Acquire the drawable early so it's available for the frame
    CAMetalLayer* metal_layer = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(internal::metal_view);
    internal::current_drawable = [metal_layer nextDrawable];
}

void device::end_frame()
{
    XS_PROFILE_FUNCTION();
    
    // End the render encoder if active
    if (internal::render_encoder)
    {
        [internal::render_encoder endEncoding];
        internal::render_encoder = nil;
    }
    
    // Present and commit
    if (internal::current_drawable && internal::command_buffer)
    {
        [internal::command_buffer presentDrawable:internal::current_drawable];
        [internal::command_buffer commit];
    }
    
    // Reset for next frame
    internal::command_buffer = nil;
    internal::current_drawable = nil;
}

void device::poll_events()
{
    SDL_Event event;
    internal::quit = false;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            internal::quit = true;
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            if (event.window.windowID == SDL_GetWindowID(internal::window))
            {
                internal::width = event.window.data1;
                internal::height = event.window.data2;
            }
            break;

        default:
            break;
        }

#ifdef INSPECTOR
        // Forward events to ImGui
        ImGui_Impl_ProcessEvent(&event);
#endif
    }
}

bool device::can_close()
{
    return true;
}

bool device::request_close()
{
    SDL_Event event;
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
    return true;
}

bool device::should_close()
{
    return internal::quit;
}

SDL_Window* device::get_window()
{
    return internal::window;
}

int xs::device::get_width()
{
    return internal::width;
}

int xs::device::get_height()
{
    return internal::height;
}

double device::hdpi_scaling()
{
    return SDL_GetWindowDisplayScale(internal::window);
}

void device::set_fullscreen(bool fullscreen)
{
    if (!internal::window)
        return;

    if (fullscreen)
    {
        SDL_SetWindowFullscreen(internal::window, true);
        SDL_GetWindowSize(internal::window, &internal::width, &internal::height);
    }
    else
    {
        SDL_SetWindowFullscreen(internal::window, false);
        internal::width = configuration::width() * configuration::multiplier();
        internal::height = configuration::height() * configuration::multiplier();
        SDL_SetWindowSize(internal::window, internal::width, internal::height);
    }
}

// Apple-specific: Get Metal layer from SDL window
void* xs::device::get_metal_layer()
{
    if (!internal::metal_view)
        return nullptr;

    return SDL_Metal_GetLayer(internal::metal_view);
}

// Metal internal functions for render compatibility
id<MTLDevice> xs::device::internal::get_device()
{
    return internal::metal_device;
}

CAMetalLayer* xs::device::internal::get_metal_layer()
{
    if (!internal::metal_view)
        return nullptr;
    return (__bridge CAMetalLayer*)SDL_Metal_GetLayer(internal::metal_view);
}

id<CAMetalDrawable> xs::device::internal::get_current_drawable()
{
    return internal::current_drawable;
}

id<MTLCommandQueue> xs::device::internal::get_command_queue()
{
    return internal::command_queue;
}

id<MTLCommandBuffer> xs::device::internal::get_command_buffer()
{
    return internal::command_buffer;
}

id<MTLRenderCommandEncoder> xs::device::internal::get_render_encoder()
{
    return internal::render_encoder;
}

void xs::device::internal::set_render_encoder(id<MTLRenderCommandEncoder> encoder)
{
    internal::render_encoder = encoder;
}



/*
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
    audio::initialize();
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

double device::hdpi_scaling()
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

// void xs::device::begin_frame() {}


void xs::device::set_fullscreen(bool) {}
 
 
*/


