#pragma once

#import <MetalKit/MetalKit.h>


namespace xs::device::internal
{
    // Get the view
    MTKView* get_view();

    // Get the current command queue (tied to the view). You are free to make new ones too.
    id<MTLCommandQueue> get_command_queue();

    // Get the current command buffer (tied to the view). You are free to make new ones too.
    id<MTLCommandBuffer> get_command_buffer();

    // Get the current render command encoder (tied to the view). You are free to make new ones too.
    id<MTLRenderCommandEncoder> get_render_encoder();

    // Creats a new render encoder for the frame
    void create_render_encoder();
}
