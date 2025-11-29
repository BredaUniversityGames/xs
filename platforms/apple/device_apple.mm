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
    int width = -1;
    int height = -1;
    bool quit = false;
    bool fullscreen = false;

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

    // Set window size in pixels
    int pixel_width = configuration::width() * configuration::multiplier();
    int pixel_height = configuration::height() * configuration::multiplier();
    auto metrics = xs::inspector::get_metrics();
    pixel_height += metrics.bottom_bar + metrics.top_bar;
    

    // Create window with Metal support
    // Note: We create a temporary window first to get the display scale,
    // then resize it to the correct point size for our desired pixel size
    internal::window = SDL_CreateWindow(
        configuration::title().c_str(),
        pixel_width,
        pixel_height,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!internal::window)
    {
        log::critical("SDL window could not be created: {}", SDL_GetError());
        SDL_Quit();
        assert(false);
        exit(EXIT_FAILURE);
    }

    // Now get the actual display scale and resize window to achieve desired pixel size
    float scale = hdpi_scaling();
    int point_width = static_cast<int>(pixel_width / scale);
    int point_height = static_cast<int>(pixel_height / scale);
    SDL_SetWindowSize(internal::window, point_width, point_height);
    
    // Store the actual pixel dimensions
    internal::width = pixel_width;
    internal::height = pixel_height;

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
    @autoreleasepool {
    // Create the command buffer for this frame
    internal::command_buffer = [internal::command_queue commandBuffer];
    internal::command_buffer.label = @"xs frame command buffer";

    // Acquire the drawable early so it's available for the frame
    CAMetalLayer* metal_layer = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(internal::metal_view);
    internal::current_drawable = [metal_layer nextDrawable];
    }
}

void device::end_frame()
{
    @autoreleasepool {
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

void xs::device::set_window_size(int w, int h)
{
    if(internal::fullscreen)
        return; // TODO: Not sure for the right course of action here
    
    if(SDL_SetWindowSize(internal::window, w, h))
    {
        internal::width = w;
        internal::height = h;
    }
}


double device::hdpi_scaling()
{
    return SDL_GetWindowDisplayScale(internal::window);
}

void device::set_fullscreen(bool fullscreen)
{
    if (!internal::window)
        return;
    
    internal::fullscreen = fullscreen;

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

bool device::get_fullscreen()
{
    return internal::fullscreen;
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

bool xs::device::toggle_on_top()
{
    if (!internal::window)
        return false;

    auto flags = SDL_GetWindowFlags(internal::window);
    bool is_on_top = (flags & SDL_WINDOW_ALWAYS_ON_TOP) != 0;
    is_on_top = !is_on_top;
    SDL_SetWindowAlwaysOnTop(internal::window, is_on_top);
    return is_on_top;
}
