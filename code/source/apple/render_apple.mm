#include "render.h"
#include "render_internal.h"
#include "configuration.h"
#include "profiler.h"
#include "device_apple.h"
#include "device.h"
#include "tools.h"
#include "imgui/imgui_impl_osx.h"
#include "imgui/imgui_impl_metal.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#import <MetalKit/MetalKit.h>

#import <simd/simd.h>
#import <ModelIO/ModelIO.h>
#import "shader_types.h"

using namespace xs;
using namespace xs::render::internal;
using namespace glm;
using namespace std;

namespace xs::render::internal
{
    id<MTLDevice> _device;

    // The render pipeline generated from the vertex and fragment shaders in the .metal shader file.
    id<MTLRenderPipelineState> _pipelineState;

    // The command queue used to pass commands to the device.
    id<MTLCommandQueue> _commandQueue;

    // The current size of the view, used as an input to the vertex shader.
    vector_uint2 _viewportSize;

    MTLRenderPipelineDescriptor* _pipelineStateDescriptor;

    int const                sprite_trigs_max = 21800;
    int                      sprite_trigs_count = 0;
    sprite_vtx_format        sprite_trigs_array[sprite_trigs_max * 3];

    MTLRenderPassDescriptor* imgui_render_pass_sescriptor = nullptr;
    id<MTLCommandBuffer> imgui_command_buffer;
    id<MTLRenderCommandEncoder> imgui_command_encoder;
}

void xs::render::initialize()
{
    NSError *error;
    
    MTKView* view = device::get_view();
    _device = view.device;
    
    // Save the size of the drawable to pass to the vertex shader.
    _viewportSize.x = device::get_width();
    _viewportSize.y = device::get_height();
    
    // Load all the shader files with a .metal file extension in the project.
    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];

    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertex_shader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragment_shader"];
    
    _pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    _pipelineStateDescriptor.label = @"xs sprite pipeline";
    _pipelineStateDescriptor.vertexFunction = vertexFunction;
    _pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    _pipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    
    MTLRenderPipelineColorAttachmentDescriptor *rb_attachment = _pipelineStateDescriptor.colorAttachments[0];
    rb_attachment.blendingEnabled = YES;
    rb_attachment.rgbBlendOperation = MTLBlendOperationAdd;
    rb_attachment.alphaBlendOperation = MTLBlendOperationAdd;
    rb_attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    rb_attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    _pipelineState = [_device newRenderPipelineStateWithDescriptor:_pipelineStateDescriptor error:&error];
    
    // Pipeline State creation could fail if the pipeline descriptor isn't set up properly.
    //  If the Metal API validation is enabled, you can find out more information about what
    //  went wrong.  (Metal API validation is enabled by default when a debug build is run
    //  from Xcode.)
    assert(_pipelineState);

    // Create the command queue
    _commandQueue = [_device newCommandQueue];
}

void xs::render::shutdown()
{
    ImGui_ImplMetal_Shutdown();
}

void xs::render::render()
{
    XS_PROFILE_SECTION("xs::render::render");
        
    MTKView* view = device::get_view();

    auto w = device::get_width() / 4.0f;
    auto h = device::get_height() / 4.0f;
    glm::mat4 p = glm::ortho(-w, w, -h, h, -100.0f, 100.0f);
    glm::mat4 v = glm::lookAt(vec3(0.0f, 0.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = p * v;
    
    // TODO: Enable alpha blending
    
    // Sort by depth
    std::stable_sort(sprite_queue.begin(), sprite_queue.end(),
        [](const sprite_queue_entry& lhs, const sprite_queue_entry& rhs) {
            return lhs.z < rhs.z;
        });
    
    // Create a new command buffer for each render pass to the current drawable.
    id<MTLCommandBuffer> command_buffer = [_commandQueue commandBuffer];
    command_buffer.label = @"xs command buffer";
    
    // Obtain a renderPassDescriptor generated from the view's drawable textures.
    MTLRenderPassDescriptor *render_pass_descriptor = view.currentRenderPassDescriptor;

    if(render_pass_descriptor == nil)
        return;
    
    // Create a render command encoder.
    id<MTLRenderCommandEncoder> render_encoder =
    [command_buffer renderCommandEncoderWithDescriptor:render_pass_descriptor];
    render_encoder.label = @"xs render encoder";

    // Set the region of the drawable to draw into.
    [render_encoder setViewport:(MTLViewport){0, 0, static_cast<double>(_viewportSize.x), static_cast<double>(_viewportSize.y), 0.0, 1.0 }];
    
    [render_encoder setRenderPipelineState:_pipelineState];

    int count = 0;
    for (auto i = 0; i < sprite_queue.size(); i++)
    {
        const auto& spe = sprite_queue[i];
        const auto& sprite = sprites[spe.sprite_id];
        const auto& image = images[sprite.image_id];

        auto from_x = 0.0;
        auto from_y = 0.0;
        auto to_x = image.width * (sprite.to.x - sprite.from.x) * spe.scale;
        auto to_y = image.height * (sprite.to.y - sprite.from.y) * spe.scale;

        auto from_u = sprite.from.x;
        auto from_v = sprite.from.y;
        auto to_u = sprite.to.x;
        auto to_v = sprite.to.y;

        if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::flip_x))
            std::swap(from_u, to_u);

        if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::flip_y))
            std::swap(from_v, to_v);

        vec4 add_color = to_vec4(spe.add_color);
        vec4 mul_color = to_vec4(spe.mul_color);
        
        sprite_trigs_array[count + 0].position = { from_x, from_y, 0.0, 1.0f };
        sprite_trigs_array[count + 1].position = { from_x, to_y, 0.0, 1.0f };
        sprite_trigs_array[count + 2].position = { to_x, to_y, 0.0, 1.0f };
        sprite_trigs_array[count + 3].position = { to_x, to_y, 0.0, 1.0f };
        sprite_trigs_array[count + 4].position = { to_x, from_y, 0.0, 1.0f };
        sprite_trigs_array[count + 5].position = { from_x, from_y, 0.0, 1.0f };

         
        sprite_trigs_array[count + 0].texture = { from_u, to_v };
        sprite_trigs_array[count + 1].texture = { from_u, from_v };
        sprite_trigs_array[count + 2].texture = { to_u, from_v };
        sprite_trigs_array[count + 3].texture = { to_u, from_v };
        sprite_trigs_array[count + 4].texture = { to_u, to_v };
        sprite_trigs_array[count + 5].texture = { from_u, to_v };

        for (int i = 0; i < 6; ++i)
        {
            sprite_trigs_array[count + i].add_color = add_color;
            sprite_trigs_array[count + i].mul_color = mul_color;
        }
            
        vec3 anchor(0.0f, 0.0f, 0.0f);
        if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center_x))
            anchor.x = (float)((to_x - from_x) * 0.5);
        if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center_y))
            anchor.y = (float)((to_y - from_y) * 0.5);
        else if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::top))
            anchor.y = (float)(to_y - from_y);

        if (anchor.x != 0.0f || anchor.y != 0.0f)
        {
            for (int i = 0; i < 6; i++)
                sprite_trigs_array[count + i].position -= vec4(anchor, 0.0f);
        }

        if (spe.rotation != 0.0)
        {
            for (int i = 0; i < 6; i++)
                rotate_vector3d(sprite_trigs_array[count + i].position, (float)spe.rotation);
        }

        for (int i = 0; i < 6; i++)
        {
            sprite_trigs_array[count + i].position.x += (float)spe.x;
            sprite_trigs_array[count + i].position.y += (float)spe.y;
        }
        count += 6;
        
        bool render_batch = true;
        
        if (render_batch)
        {
            // Pass in the parameter data.
            [render_encoder setVertexBytes:sprite_trigs_array
                length:sizeof(sprite_vtx_format) * count
                atIndex:index_vertices];
            
            [render_encoder setVertexBytes:&vp
                length:sizeof(mat4)
                atIndex:index_wvp];
            
            [render_encoder setFragmentTexture:image.texture
                atIndex:index_sprite_texture];

            // Draw the triangle.
            [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                vertexStart:0
                vertexCount:count];
            
            count = 0;
        }
    }
    
    [render_encoder endEncoding];

    // Schedule a present once the framebuffer is complete using the current drawable.
    [command_buffer presentDrawable:view.currentDrawable];

    // Finalize rendering here & push the command buffer to the GPU.
    [command_buffer commit];

    sprite_queue.clear();
}

void xs::render::clear()
{
}

void xs::render::begin(primitive p)
{
}

void xs::render::vertex(double x, double y)
{
}

void xs::render::end()
{
}

void xs::render::set_color(double r, double g, double b, double a)
{
}

void xs::render::set_color(color c)
{
}

void xs::render::line(double x0, double y0, double x1, double y1)
{}

void xs::render::text(const std::string& text, double x, double y, double size)
{}

void xs::render::internal::create_texture_with_data(
    xs::render::internal::image& img,
    uchar* data)
{
    MTLTextureDescriptor* texture_descriptor = [[MTLTextureDescriptor alloc] init];
    texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Uint; // 0-255 RGBA
    //texture_descriptor.pixelFormat = MTLPixelFormatRGBA8; // 0-255 RGBA
    texture_descriptor.width = img.width;
    texture_descriptor.height= img.height;
    img.texture = [_device newTextureWithDescriptor:texture_descriptor];
    
    MTLRegion region = {
        { 0, 0, 0 },                                                // MTLOrigin
        {texture_descriptor.width, texture_descriptor.height, 1}    // MTLSize
    };
    
    [img.texture
     replaceRegion:region
     mipmapLevel:0
     withBytes:data
     bytesPerRow:texture_descriptor.width * 4];
}

void xs::render::internal::imgui_init()
{
    auto osx = ImGui_ImplOSX_Init(xs::device::get_view());
    auto metal = ImGui_ImplMetal_Init(_device);
    assert(osx);
    assert(metal);
}

void xs::render::internal::imgui_shutdown()
{
    /*
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplOSX_Shutdown();
     */
}

void xs::render::internal::imgui_new_frame()
{
    MTKView* view = device::get_view();
    ImGui_ImplOSX_NewFrame(xs::device::get_view());
    
    // Create a new command buffer for each render pass to the current drawable.
    imgui_command_buffer = [_commandQueue commandBuffer];
    imgui_command_buffer.label = @"imgui command buffer";
    
    // Obtain a renderPassDescriptor generated from the view's drawable textures.
    imgui_render_pass_sescriptor = view.currentRenderPassDescriptor;
    ImGui_ImplMetal_NewFrame(imgui_render_pass_sescriptor);
}

void xs::render::internal::imgui_render(ImDrawData* data)
{
    /*
    MTKView* view = device::get_view();
    
    // Create a render command encoder.
    id<MTLRenderCommandEncoder> render_encoder =
    [imgui_command_buffer renderCommandEncoderWithDescriptor:imgui_render_pass_sescriptor];
    render_encoder.label = @"imgui render encoder";
    
    // Set the region of the drawable to draw into.
    [render_encoder setViewport:(MTLViewport){0, 0, static_cast<double>(_viewportSize.x), static_cast<double>(_viewportSize.y), 0.0, 1.0 }];
    
    [render_encoder setRenderPipelineState:_pipelineState];
    
    ImGui_ImplMetal_RenderDrawData(data, imgui_command_buffer, render_encoder);
    
    [render_encoder endEncoding];

    // Schedule a present once the framebuffer is complete using the current drawable.
    [imgui_command_buffer presentDrawable:view.currentDrawable];

    // Finalize rendering here & push the command buffer to the GPU.
    [imgui_command_buffer commit];
     */
}
