#pragma once

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

// Forward declare SDL_Window
struct SDL_Window;

namespace xs::device
{
    // Get SDL window
    SDL_Window* get_window();

    // Get Metal layer from SDL window (Apple-specific)
    void* get_metal_layer();
}

namespace xs::device::internal
{
    // Get the Metal device
    id<MTLDevice> get_device();

    // Get the CAMetalLayer (SDL3 uses this instead of MTKView)
    CAMetalLayer* get_metal_layer();

    // Get the current drawable for this frame
    id<CAMetalDrawable> get_current_drawable();

    // Get the current command queue. You are free to make new ones too.
    id<MTLCommandQueue> get_command_queue();

    // Get the current command buffer. You are free to make new ones too.
    id<MTLCommandBuffer> get_command_buffer();

    // Get the current render command encoder. You are free to make new ones too.
    id<MTLRenderCommandEncoder> get_render_encoder();

    // Creates a new render encoder for the frame
    void create_render_encoder();
}
