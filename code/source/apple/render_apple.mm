#include "render.h"
#include "render_internal.h"
#include "configuration.h"
#include "profiler.h"
#include "device_apple.h"
#include "device.h"
#include "tools.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#import <MetalKit/MetalKit.h>

#import <simd/simd.h>
#import <ModelIO/ModelIO.h>
#import "shader_types.h"

/*
static const NSUInteger MaxBuffersInFlight = 3;

matrix_float4x4 matrix4x4_translation(float tx, float ty, float tz)
{
    return (matrix_float4x4) {{
        { 1,   0,  0,  0 },
        { 0,   1,  0,  0 },
        { 0,   0,  1,  0 },
        { tx, ty, tz,  1 }
    }};
}

static matrix_float4x4 matrix4x4_rotation(float radians, vector_float3 axis)
{
    axis = vector_normalize(axis);
    float ct = cosf(radians);
    float st = sinf(radians);
    float ci = 1 - ct;
    float x = axis.x, y = axis.y, z = axis.z;

    return (matrix_float4x4) {{
        { ct + x * x * ci,     y * x * ci + z * st, z * x * ci - y * st, 0},
        { x * y * ci - z * st,     ct + y * y * ci, z * y * ci + x * st, 0},
        { x * z * ci + y * st, y * z * ci - x * st,     ct + z * z * ci, 0},
        {                   0,                   0,                   0, 1}
    }};
}

matrix_float4x4 matrix_perspective_right_hand(float fovyRadians, float aspect, float nearZ, float farZ)
{
    float ys = 1 / tanf(fovyRadians * 0.5);
    float xs = ys / aspect;
    float zs = farZ / (nearZ - farZ);

    return (matrix_float4x4) {{
        { xs,   0,          0,  0 },
        {  0,  ys,          0,  0 },
        {  0,   0,         zs, -1 },
        {  0,   0, nearZ * zs,  0 }
    }};
}
*/

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

    /*
    struct sprite_vtx_format
    {
        vec3 position;
        vec2 texture;
        vec4 add_color;
        vec4 mul_color;
    };
    */

    int const                sprite_trigs_max = 21800;
    int                      sprite_trigs_count = 0;
    sprite_vtx_format        sprite_trigs_array[sprite_trigs_max * 3];

/*

    dispatch_semaphore_t _inFlightSemaphore;
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;

    id <MTLBuffer> _dynamicUniformBuffer[MaxBuffersInFlight];
    id <MTLRenderPipelineState> _pipelineState;
    id <MTLDepthStencilState> _depthState;
    id <MTLTexture> _colorMap;
    MTLVertexDescriptor *_mtlVertexDescriptor;

    uint8_t _uniformBufferIndex;

    matrix_float4x4 _projectionMatrix;

    float _rotation;

    MTKMesh *_mesh;
 
*/
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
    
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"xs sprite pipeline";
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    
    MTLRenderPipelineColorAttachmentDescriptor *rb_attachment = pipelineStateDescriptor.colorAttachments[0];
    rb_attachment.blendingEnabled = YES;
    rb_attachment.rgbBlendOperation = MTLBlendOperationAdd;
    rb_attachment.alphaBlendOperation = MTLBlendOperationAdd;
    rb_attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    rb_attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    
    // Pipeline State creation could fail if the pipeline descriptor isn't set up properly.
    //  If the Metal API validation is enabled, you can find out more information about what
    //  went wrong.  (Metal API validation is enabled by default when a debug build is run
    //  from Xcode.)
    assert(_pipelineState);

    // Create the command queue
    _commandQueue = [_device newCommandQueue];

    /*
    _inFlightSemaphore = dispatch_semaphore_create(MaxBuffersInFlight);
    
    
    /// Load Metal state objects and initialize renderer dependent view properties

    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    view.sampleCount = 1;

    _mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];

    _mtlVertexDescriptor.attributes[VertexAttributePosition].format = MTLVertexFormatFloat3;
    _mtlVertexDescriptor.attributes[VertexAttributePosition].offset = 0;
    _mtlVertexDescriptor.attributes[VertexAttributePosition].bufferIndex = BufferIndexMeshPositions;

    _mtlVertexDescriptor.attributes[VertexAttributeTexcoord].format = MTLVertexFormatFloat2;
    _mtlVertexDescriptor.attributes[VertexAttributeTexcoord].offset = 0;
    _mtlVertexDescriptor.attributes[VertexAttributeTexcoord].bufferIndex = BufferIndexMeshGenerics;

    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stride = 12;
    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stepRate = 1;
    _mtlVertexDescriptor.layouts[BufferIndexMeshPositions].stepFunction = MTLVertexStepFunctionPerVertex;

    _mtlVertexDescriptor.layouts[BufferIndexMeshGenerics].stride = 8;
    _mtlVertexDescriptor.layouts[BufferIndexMeshGenerics].stepRate = 1;
    _mtlVertexDescriptor.layouts[BufferIndexMeshGenerics].stepFunction = MTLVertexStepFunctionPerVertex;

    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];

    id <MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];

    id <MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];

    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"MyPipeline";
    pipelineStateDescriptor.rasterSampleCount = view.sampleCount;
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    pipelineStateDescriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;
    pipelineStateDescriptor.stencilAttachmentPixelFormat = view.depthStencilPixelFormat;

    NSError *error = NULL;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;
    _depthState = [_device newDepthStencilStateWithDescriptor:depthStateDesc];

    for(NSUInteger i = 0; i < MaxBuffersInFlight; i++)
    {
        _dynamicUniformBuffer[i] = [_device newBufferWithLength:sizeof(Uniforms)
                                                        options:MTLResourceStorageModeShared];

        _dynamicUniformBuffer[i].label = @"UniformBuffer";
    }

    _commandQueue = [_device newCommandQueue];
    
    
    
    { // Load assets
        NSError *error;

        MTKMeshBufferAllocator *metalAllocator = [[MTKMeshBufferAllocator alloc]
                                                  initWithDevice: _device];

        MDLMesh *mdlMesh = [MDLMesh newBoxWithDimensions:(vector_float3){4, 4, 4}
                                                segments:(vector_uint3){2, 2, 2}
                                            geometryType:MDLGeometryTypeTriangles
                                           inwardNormals:NO
                                               allocator:metalAllocator];

        MDLVertexDescriptor *mdlVertexDescriptor =
        MTKModelIOVertexDescriptorFromMetal(_mtlVertexDescriptor);

        mdlVertexDescriptor.attributes[VertexAttributePosition].name  = MDLVertexAttributePosition;
        mdlVertexDescriptor.attributes[VertexAttributeTexcoord].name  = MDLVertexAttributeTextureCoordinate;

        mdlMesh.vertexDescriptor = mdlVertexDescriptor;

        _mesh = [[MTKMesh alloc] initWithMesh:mdlMesh
                                       device:_device
                                        error:&error];

        if(!_mesh || error)
        {
            NSLog(@"Error creating MetalKit mesh %@", error.localizedDescription);
        }

        MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:_device];

        NSDictionary *textureLoaderOptions =
        @{
          MTKTextureLoaderOptionTextureUsage       : @(MTLTextureUsageShaderRead),
          MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
          };

        _colorMap = [textureLoader newTextureWithName:@"ColorMap"
                                          scaleFactor:1.0
                                               bundle:nil
                                              options:textureLoaderOptions
                                                error:&error];
        
        if(!_colorMap || error)
        {
            NSLog(@"Error creating texture %@", error.localizedDescription);
        }
        
    }
     */
}

void xs::render::shutdown()
{
}

void xs::render::render()
{
    /*
    XS_PROFILE_SECTION("xs::render::render");
    
    MTKView* view = device::get_view();
    
    /// Per frame updates here

    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);

    _uniformBufferIndex = (_uniformBufferIndex + 1) % MaxBuffersInFlight;

    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    __block dispatch_semaphore_t block_sema = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
         dispatch_semaphore_signal(block_sema);
     }];

    {
        /// Update any game state before encoding renderint commands to our drawable

        Uniforms * uniforms = (Uniforms*)_dynamicUniformBuffer[_uniformBufferIndex].contents;
        
        float aspect = device::get_width() / (float)device::get_height();
        _projectionMatrix = matrix_perspective_right_hand(65.0f * (M_PI / 180.0f), aspect, 0.1f, 100.0f);

        uniforms->projectionMatrix = _projectionMatrix;

        vector_float3 rotationAxis = {1, 1, 0};
        matrix_float4x4 modelMatrix = matrix4x4_rotation(_rotation, rotationAxis);
        matrix_float4x4 viewMatrix = matrix4x4_translation(0.0, 0.0, -8.0);

        uniforms->modelViewMatrix = matrix_multiply(viewMatrix, modelMatrix);

        _rotation += .01;
    }

    /// Delay getting the currentRenderPassDescriptor until absolutely needed. This avoids
    ///   holding onto the drawable and blocking the display pipeline any longer than necessary
    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;

    if(renderPassDescriptor != nil)
    {
        /// Final pass rendering code here

        id <MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";

        [renderEncoder pushDebugGroup:@"DrawBox"];

        [renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        [renderEncoder setCullMode:MTLCullModeBack];
        [renderEncoder setRenderPipelineState:_pipelineState];
        [renderEncoder setDepthStencilState:_depthState];

        [renderEncoder setVertexBuffer:_dynamicUniformBuffer[_uniformBufferIndex]
                                offset:0
                               atIndex:BufferIndexUniforms];

        [renderEncoder setFragmentBuffer:_dynamicUniformBuffer[_uniformBufferIndex]
                                  offset:0
                                 atIndex:BufferIndexUniforms];

        for (NSUInteger bufferIndex = 0; bufferIndex < _mesh.vertexBuffers.count; bufferIndex++)
        {
            MTKMeshBuffer *vertexBuffer = _mesh.vertexBuffers[bufferIndex];
            if((NSNull*)vertexBuffer != [NSNull null])
            {
                [renderEncoder setVertexBuffer:vertexBuffer.buffer
                                        offset:vertexBuffer.offset
                                       atIndex:bufferIndex];
            }
        }

        [renderEncoder setFragmentTexture:_colorMap
                                  atIndex:TextureIndexColor];

        for(MTKSubmesh *submesh in _mesh.submeshes)
        {
            [renderEncoder drawIndexedPrimitives:submesh.primitiveType
                                      indexCount:submesh.indexCount
                                       indexType:submesh.indexType
                                     indexBuffer:submesh.indexBuffer.buffer
                               indexBufferOffset:submesh.indexBuffer.offset];
        }

        [renderEncoder popDebugGroup];

        [renderEncoder endEncoding];

        [commandBuffer presentDrawable:view.currentDrawable];
    }

    [commandBuffer commit];
     */
    
    
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

        // TODO: Set texture

        /*
        sprite_trigs_array[count + 0].position = { from_x, from_y, 0.0 };
        sprite_trigs_array[count + 1].position = { from_x, to_y, 0.0 };
        sprite_trigs_array[count + 2].position = { to_x, to_y, 0.0 };
        sprite_trigs_array[count + 3].position = { to_x, to_y, 0.0 };
        sprite_trigs_array[count + 4].position = { to_x, from_y, 0.0 };
        sprite_trigs_array[count + 5].position = { from_x, from_y, 0.0 };
        */
        
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

        /*
        bool render_batch = false;
        if (i < sprite_queue.size() - 1 && count < 32)
        {
            const auto& nspe = sprite_queue[i + 1];
            const auto& nsprite = sprites[nspe.sprite_id];
            render_batch = nsprite.image_id != sprite.image_id;
        }
        else
        {
            render_batch = true;
        }
        */
        
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
