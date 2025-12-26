#include "render.hpp"
#include "render.hpp"
#include "render_apple.h"
#include "render_internal.hpp"
#include "configuration.hpp"
#include "profiler.hpp"
#include "device_apple.h"
#include "device.hpp"
#include "tools.hpp"
#include "log.hpp"
#include "inspector.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <simd/simd.h>
#import <ModelIO/ModelIO.h>
#import "shader_types.h"

#define XS_QUANTIZED_HASHING

using namespace glm;
using namespace std;
using namespace xs;
using namespace xs::render;

namespace xs::render
{
    id<MTLDevice> _device;

    //id<MTLRenderPipelineState> _pipelineStateDescriptor;

    // The render pipeline generated from the vertex and fragment shaders in the .metal shader file.
    id<MTLRenderPipelineState> _pipelineState;

    // The command queue used to pass commands to the device.
    // id<MTLCommandQueue> _commandQueue;

    // The current size of the view, used as an input to the vertex shader.
    vector_uint2 _viewportSize;

    // Triple buffering: 3 textures to render to and then sample from
    static const int BUFFER_COUNT = 3;
    id<MTLTexture> _renderTargetTextures[BUFFER_COUNT];       // Resolved textures (for sampling)
    id<MTLTexture> _msaaTextures[BUFFER_COUNT];               // MSAA render targets (8x)
    id<MTLTexture> _depthTexture = nil;                       // Shared depth buffer for MSAA
    int _currentTextureIndex = 0;
    id<MTLTexture> _currentRenderTarget = nil;  // Updated each frame (resolved texture)

    // Render pass descriptor to draw to the texture
    MTLRenderPassDescriptor* _renderToTextureRenderPassDescriptor;

    // Mesh-based sprite rendering pipeline
    id<MTLRenderPipelineState> _meshRenderPipeline;

    id<MTLRenderPipelineState> _debugRenderPipeline;

    MTLRenderPipelineDescriptor* _pipelineStateDescriptor;

    void render_to_view();


	// Metal mesh structure for persistent GPU buffers
	struct metal_mesh
	{
		id<MTLBuffer> vertex_buffer = nil;      // Position data
		id<MTLBuffer> texcoord_buffer = nil;    // UV coordinates
		id<MTLBuffer> index_buffer = nil;       // Indices for triangles
		uint32_t index_count = 0;               // Number of indices
		int image_id = -1;                      // Associated texture
		vec4 xy = {};                           // Sprite bounds (min_x, min_y, max_x, max_y)
		vec4 uv = {};                           // Texture coords (u0, v0, u1, v1)
		bool is_sprite = false;                 // Sprite vs custom shape
	};

	struct sprite_queue_entry
	{
		int sprite_id	= -1;
		double			x = 0.0;
		double			y = 0.0;
		double			z = 0.0;
		double			scale = 1.0;
		double			rotation = 0.0;
		color			mul_color = {};
		color			add_color = {};
		unsigned int	flags = 0;
	};

	// Mesh storage - persistent GPU buffers
	std::unordered_map<int, metal_mesh> meshes;

	std::vector<sprite_queue_entry>	sprite_queue = {};

}

void xs::render::initialize()
{
    NSError *error;
    
    CAMetalLayer* metalLayer = device::internal::get_metal_layer();
    _device = device::internal::get_device();
    
    // Save the size of the drawable to pass to the vertex shader.
    _viewportSize.x = configuration::width();
    _viewportSize.y = configuration::height();
    
    // Load all the shader files with a .metal file extension in the project.
    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];

    // Render to texture
    {
        NSError *error;

        // Set up a texture for rendering to and sampling from
		NSUInteger scale = 1;
		if(!configuration::snap_to_pixels())
		{
			scale *= configuration::multiplier();
			if(configuration::window_size_in_points())
			{
				scale *= device::hdpi_scaling();
			}
		}

        MTLTextureDescriptor *texDescriptor = [MTLTextureDescriptor new];
        texDescriptor.textureType = MTLTextureType2D;
        texDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
		texDescriptor.width = configuration::width() * scale;
		texDescriptor.height = configuration::height() * scale;
        texDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;

        // Create all 3 resolved textures for triple buffering (these will be sampled)
        for (int i = 0; i < BUFFER_COUNT; i++) {
            _renderTargetTextures[i] = [_device newTextureWithDescriptor:texDescriptor];
            assert(_renderTargetTextures[i]);
        }

        // Create MSAA textures (8x multisampling for antialiasing)
        // Check device capabilities and use maximum supported sample count
        NSUInteger sampleCount = 8;
        if (![_device supportsTextureSampleCount:sampleCount]) {
            // Try 4x if 8x not supported
            sampleCount = 4;
            if (![_device supportsTextureSampleCount:sampleCount]) {
                sampleCount = 1; // Fall back to no MSAA
            }
        }

        MTLTextureDescriptor *msaaDescriptor = [MTLTextureDescriptor new];
        msaaDescriptor.textureType = MTLTextureType2DMultisample;
        msaaDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
        msaaDescriptor.width = configuration::width() * scale;
        msaaDescriptor.height = configuration::height() * scale;
        msaaDescriptor.sampleCount = sampleCount;
        msaaDescriptor.usage = MTLTextureUsageRenderTarget;
        msaaDescriptor.storageMode = MTLStorageModePrivate;

        for (int i = 0; i < BUFFER_COUNT; i++) {
            _msaaTextures[i] = [_device newTextureWithDescriptor:msaaDescriptor];
            if (!_msaaTextures[i]) {
                log::critical("Failed to create MSAA texture with sample count {}", sampleCount);
            }
            assert(_msaaTextures[i]);
        }

        log::info("Metal renderer using {}x MSAA", sampleCount);

        // Create depth buffer for MSAA (shared across all frames)
        MTLTextureDescriptor *depthDescriptor = [MTLTextureDescriptor new];
        depthDescriptor.textureType = MTLTextureType2DMultisample;
        depthDescriptor.pixelFormat = MTLPixelFormatDepth32Float;
        depthDescriptor.width = configuration::width() * scale;
        depthDescriptor.height = configuration::height() * scale;
        depthDescriptor.sampleCount = sampleCount;
        depthDescriptor.usage = MTLTextureUsageRenderTarget;
        depthDescriptor.storageMode = MTLStorageModePrivate;

        _depthTexture = [_device newTextureWithDescriptor:depthDescriptor];
        if (!_depthTexture) {
            log::critical("Failed to create depth texture");
        }
        assert(_depthTexture);

        // Initialize render pass descriptor for MSAA (textures will be set per-frame)
        _renderToTextureRenderPassDescriptor = [MTLRenderPassDescriptor new];
        _renderToTextureRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        _renderToTextureRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);  // Transparent clear
        _renderToTextureRenderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;  // Resolve MSAA

        // Depth attachment
        _renderToTextureRenderPassDescriptor.depthAttachment.texture = _depthTexture;
        _renderToTextureRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        _renderToTextureRenderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
        _renderToTextureRenderPassDescriptor.depthAttachment.clearDepth = 1.0;

        // Debug rendering pipeline
        id<MTLFunction> vertexDebugFunction = [defaultLibrary newFunctionWithName:@"vertex_shader_debug"];
        id<MTLFunction> fragmentDebugFunction = [defaultLibrary newFunctionWithName:@"fragment_shader_debug"];

        MTLRenderPipelineDescriptor *debugStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        debugStateDescriptor.label = @"xs debug render pipeline";
        debugStateDescriptor.rasterSampleCount = sampleCount;
        debugStateDescriptor.vertexFunction = vertexDebugFunction;
        debugStateDescriptor.fragmentFunction = fragmentDebugFunction;
        debugStateDescriptor.colorAttachments[0].pixelFormat = _renderTargetTextures[0].pixelFormat;
        debugStateDescriptor.colorAttachments[0].blendingEnabled = YES;
        debugStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        debugStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        debugStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        debugStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        debugStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        _debugRenderPipeline = [_device newRenderPipelineStateWithDescriptor:debugStateDescriptor error:&error];
        assert(_debugRenderPipeline);

        // Mesh-based sprite rendering pipeline
        id<MTLFunction> vertexMeshFunction = [defaultLibrary newFunctionWithName:@"vertex_shader_mesh"];
        id<MTLFunction> fragmentMeshFunction = [defaultLibrary newFunctionWithName:@"fragment_shader"];

        MTLRenderPipelineDescriptor *meshStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        meshStateDescriptor.label = @"xs mesh render pipeline";
        meshStateDescriptor.rasterSampleCount = sampleCount;
        meshStateDescriptor.vertexFunction = vertexMeshFunction;
        meshStateDescriptor.fragmentFunction = fragmentMeshFunction;
        meshStateDescriptor.colorAttachments[0].pixelFormat = _renderTargetTextures[0].pixelFormat;
        meshStateDescriptor.colorAttachments[0].blendingEnabled = YES;
        meshStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        meshStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        meshStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        meshStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        meshStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        meshStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        meshStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        _meshRenderPipeline = [_device newRenderPipelineStateWithDescriptor:meshStateDescriptor error:&error];
        assert(_meshRenderPipeline);
    }

    // Render to view
    {
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertex_shader_screen"];
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragment_shader_screen"];

        _pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        _pipelineStateDescriptor.label = @"xs render to screen pipeline";
        _pipelineStateDescriptor.vertexFunction = vertexFunction;
        _pipelineStateDescriptor.fragmentFunction = fragmentFunction;
        _pipelineStateDescriptor.colorAttachments[0].pixelFormat = metalLayer.pixelFormat;
        
        _pipelineState = [_device newRenderPipelineStateWithDescriptor:_pipelineStateDescriptor error:&error];
        assert(_pipelineState);
    }
}

void xs::render::shutdown()
{
    // TODO: Cleanup as needed
}

void xs::render::render()
{
    @autoreleasepool {
    XS_PROFILE_SECTION("xs::render::render");

    // Rotate to next texture for triple buffering
    _currentTextureIndex = (_currentTextureIndex + 1) % BUFFER_COUNT;
    _currentRenderTarget = _renderTargetTextures[_currentTextureIndex];

    // Update render pass descriptor for MSAA rendering
    // Render to MSAA texture, resolve to regular texture for sampling
    _renderToTextureRenderPassDescriptor.colorAttachments[0].texture = _msaaTextures[_currentTextureIndex];
    _renderToTextureRenderPassDescriptor.colorAttachments[0].resolveTexture = _currentRenderTarget;

    // MTKView* view = device::internal::get_view();

    auto w = configuration::width() * 0.5f;
    auto h = configuration::height() * 0.5f;
    glm::mat4 p = glm::ortho(-w, w, -h, h, -100.0f, 100.0f);
    glm::mat4 v = glm::lookAt(vec3(0.0f, 0.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = p * v;

    // Sort by depth
    std::stable_sort(sprite_queue.begin(), sprite_queue.end(),
        [](const sprite_queue_entry& lhs, const sprite_queue_entry& rhs) {
            return lhs.z < rhs.z;
        });

    // Create a new command buffer for each render pass to the current drawable.
    id<MTLCommandBuffer> command_buffer = device::internal::get_command_buffer();

    id<MTLRenderCommandEncoder> render_encoder =
            [command_buffer renderCommandEncoderWithDescriptor:_renderToTextureRenderPassDescriptor];
    render_encoder.label = @"xs offscreen render pass";
    [render_encoder setRenderPipelineState:_meshRenderPipeline];

    // Render all sprites using mesh buffers
    for (auto i = 0; i < sprite_queue.size(); i++)
    {
        const auto& spe = sprite_queue[i];

        // Look up mesh
        auto mesh_it = meshes.find(spe.sprite_id);
        if (mesh_it == meshes.end()) {
            log::error("Sprite {} not found in mesh cache!", spe.sprite_id);
            continue;
        }

        const metal_mesh& mesh = mesh_it->second;
        const auto& image = images[mesh.image_id];

        // Create instance data for vertex shader
        sprite_instance_data instance;
        instance.xy = mesh.xy;
        instance.uv = mesh.uv;
        instance.position = vec2((float)spe.x, (float)spe.y);
        instance.scale = vec2((float)spe.scale, (float)spe.scale);
        instance.rotation = (float)spe.rotation;
        instance.mul_color = to_vec4(spe.mul_color);
        instance.add_color = to_vec4(spe.add_color);
        instance.flags = spe.flags;

        // Bind mesh buffers
        [render_encoder setVertexBuffer:mesh.vertex_buffer
            offset:0
            atIndex:index_vertices];

        [render_encoder setVertexBuffer:mesh.texcoord_buffer
            offset:0
            atIndex:index_texcoords];

        // Bind transform/color data
        [render_encoder setVertexBytes:&vp
            length:sizeof(mat4)
            atIndex:index_wvp];

        [render_encoder setVertexBytes:&instance
            length:sizeof(sprite_instance_data)
            atIndex:index_instance];

        // Bind texture
        [render_encoder setFragmentTexture:image.texture
            atIndex:index_sprite_texture];

        // Draw indexed mesh
        [render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
            indexCount:mesh.index_count
            indexType:MTLIndexTypeUInt16
            indexBuffer:mesh.index_buffer
            indexBufferOffset:0];
    }
        
    if(triangles_count > 0)
    {
        [render_encoder setRenderPipelineState:_debugRenderPipeline];
                
        int to_draw = triangles_count;
        int idx = 0;
        while(to_draw > 0)
        {
            int count = std::min(to_draw, 32);
            to_draw -= count;
            
            [render_encoder setVertexBytes:&triangles_array[idx]
                length:sizeof(debug_vtx_format) * count * 3
                atIndex:index_vertices];
                        
            [render_encoder setVertexBytes:&vp
                length:sizeof(mat4)
                atIndex:index_wvp];
            
            // Draw the triangles
            [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                vertexStart:0
                vertexCount:count * 3];
            
            idx += count * 3;
        }
        
        triangles_count = 0;
    }
    
    if(lines_count > 0)
    {
        [render_encoder setRenderPipelineState:_debugRenderPipeline];
        int to_draw = lines_count;
        int idx = 0;
        while(to_draw > 0)
        {
            int count = std::min(to_draw, 32);
            to_draw -= count;
            
            [render_encoder setVertexBytes:&lines_array[idx]
                length:sizeof(debug_vtx_format) * count * 2
                atIndex:index_vertices];
                        
            [render_encoder setVertexBytes:&vp
                length:sizeof(mat4)
                atIndex:index_wvp];
            
            // Draw the triangles
            [render_encoder drawPrimitives:MTLPrimitiveTypeLine
                vertexStart:0
                vertexCount:count * 2];
            
            idx += count * 2;
        }
        
        lines_count = 0;
    }
    
    [render_encoder endEncoding];

#ifdef INSPECTOR
    // When using inspector, add a blit encoder to ensure the render target is finished
    // before ImGui tries to sample it
    id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
    [blit_encoder synchronizeResource:_currentRenderTarget];
    [blit_encoder endEncoding];
#endif

    // Composite offscreen texture to screen
    id<CAMetalDrawable> drawable = device::internal::get_current_drawable();
    if (drawable != nil)
    {
        // Create screen render pass
        MTLRenderPassDescriptor* screen_rpd = [MTLRenderPassDescriptor renderPassDescriptor];
        screen_rpd.colorAttachments[0].texture = drawable.texture;
        screen_rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
        screen_rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
        screen_rpd.colorAttachments[0].clearColor = MTLClearColorMake(1.0, 1.0, 1.0, 1.0);
        id<MTLRenderCommandEncoder> screen_encoder = [command_buffer renderCommandEncoderWithDescriptor:screen_rpd];
        screen_encoder.label = @"xs screen render pass";

#ifndef INSPECTOR
        // When inspector is disabled, draw the game texture directly to screen
        const float dw = device::get_width();
        const float dh = device::get_height();
        const float cw = configuration::width();
        const float ch = configuration::height();

        screen_vtx_format quadVertices[6];
        if(device::get_fullscreen())
        {
            const float aspect = (cw / ch) / (dw / dh);
            float qw = aspect;
            float qh = 1.0f;
            //                Positions      , Texture coordinates
            quadVertices[0] = { {  qw,  -qh },  { 1.0, 1.0 } };
            quadVertices[1] = { { -qw,  -qh },  { 0.0, 1.0 } };
            quadVertices[2] = { { -qw,   qh },  { 0.0, 0.0 } };
            quadVertices[3] = { {  qw,  -qh },  { 1.0, 1.0 } };
            quadVertices[4] = { { -qw,   qh },  { 0.0, 0.0 } };
            quadVertices[5] = { {  qw,   qh },  { 1.0, 0.0 } };
        }
        else
        {
            vec2 fr(0.0f, 0.0f);
            vec2 to(cw, ch);
            //                Positions      , Texture coordinates
            quadVertices[0] = { { to.x, to.y },  { 1.0, 0.0 } };
            quadVertices[1] = { { fr.x, to.y },  { 0.0, 0.0 } };
            quadVertices[2] = { { fr.x, fr.y },  { 0.0, 1.0 } };
            quadVertices[3] = { { to.x, to.y },  { 1.0, 0.0 } };
            quadVertices[4] = { { fr.x, fr.y },  { 0.0, 1.0 } };
            quadVertices[5] = { { to.x, fr.y },  { 1.0, 1.0 } };
        }

        [screen_encoder setRenderPipelineState:_pipelineState];
        [screen_encoder setVertexBytes:&quadVertices
                                length:sizeof(quadVertices)
                               atIndex:index_vertices];

        vec2 resolution(dw, dh);
        [screen_encoder setVertexBytes:&resolution length:sizeof(vec2) atIndex:index_resolution];

        // Set the offscreen texture as the source texture (from triple buffering pool)
        [screen_encoder setFragmentTexture:_currentRenderTarget atIndex:index_sprite_texture];

        // Draw quad with rendered texture.
        [screen_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                           vertexStart:0
                           vertexCount:6];
#endif

        // Store the screen encoder for ImGui to use (when inspector is enabled)
        // end_frame() will call endEncoding
        device::internal::set_render_encoder(screen_encoder);
    }
    } // @autoreleasepool
}

void xs::render::clear()
{
    sprite_queue.clear();
}

void xs::render::create_texture_with_data(
    xs::render::image& img,
    uchar* data)
{
    @autoreleasepool {
    MTLTextureDescriptor* texture_descriptor = [[MTLTextureDescriptor alloc] init];
    texture_descriptor.pixelFormat = MTLPixelFormatRGBA8Uint;   // 0-255 RGBA
    //texture_descriptor.pixelFormat = MTLPixelFormatRGBA8;     // 0-255 RGBA
    texture_descriptor.width = img.width;
    texture_descriptor.height= img.height;
    texture_descriptor.usage = MTLTextureUsageShaderRead;
    texture_descriptor.storageMode = MTLStorageModeShared;
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
}

int xs::render::create_shape(
	int image_id,
	const float *positions,
	const float *texture_coordinates,
	unsigned int vertex_count,
	const unsigned short *indices,
	unsigned int index_count)
{
	if (image_id < 0 || image_id >= images.size()) {
		log::error("Can't create shape with image {}!", image_id);
		return -1;
	}

	// Generate unique ID for custom shapes (not cached like sprites)
	static int next_shape_id = 1000000;  // Start high to avoid collision with sprite hashes
	int shape_id = next_shape_id++;

	// Create Metal buffers for arbitrary geometry
	metal_mesh mesh;

	mesh.vertex_buffer = [_device newBufferWithBytes:positions
	                                          length:vertex_count * 2 * sizeof(float)
	                                         options:MTLResourceStorageModeShared];

	mesh.texcoord_buffer = [_device newBufferWithBytes:texture_coordinates
	                                            length:vertex_count * 2 * sizeof(float)
	                                           options:MTLResourceStorageModeShared];

	mesh.index_buffer = [_device newBufferWithBytes:indices
	                                         length:index_count * sizeof(unsigned short)
	                                        options:MTLResourceStorageModeShared];

	mesh.index_count = index_count;
	mesh.image_id = image_id;
	mesh.xy = vec4(0, 0, 0, 0);  // Not used for custom shapes
	mesh.uv = vec4(0, 0, 1, 1);  // Full texture by default
	mesh.is_sprite = false;

	// Store mesh
	meshes[shape_id] = mesh;

	return shape_id;
}

void xs::render::destroy_shape(int sprite_id)
{
	auto it = meshes.find(sprite_id);
	if (it == meshes.end()) {
		return;  // Mesh doesn't exist
	}

	// Metal buffers are automatically released when ARC decrements ref count
	// Just remove from map
	meshes.erase(it);
}

int xs::render::create_sprite(int image_id, double x0, double y0, double x1, double y1)
{
	if (image_id < 0 || image_id >= images.size()) {
		log::error("Can't create sprite with image {}!", image_id);
		return -1;
	}

#if defined(XS_QUANTIZED_HASHING)
	// Precision for the texture coordinates
	double precision = 10000.0;
	int xh0 = (int)(x0 * precision);
	int yh0 = (int)(y0 * precision);
	int xh1 = (int)(x1 * precision);
	int yh1 = (int)(y1 * precision);
	auto key = tools::hash_combine(image_id, xh0, yh0, xh1, yh1);
#else
	// Check if the sprite already exists
	auto key = tools::hash_combine(image_id, x0, y0, x1, y1);
#endif

	auto it = meshes.find(key);
	if (it != meshes.end())
		return it->first;

	// Get image dimensions
	const auto& img = images[image_id];
	float img_w = (float)img.width;
	float img_h = (float)img.height;

	// Input coordinates are normalized (0-1), convert to pixel dimensions for mesh.xy
	float from_x = 0.0f;
	float from_y = 0.0f;
	float to_x = img_w * (float)(x1 - x0);
	float to_y = img_h * (float)(y1 - y0);

	// UV coordinates: normalize the input coords (which are already 0-1 range)
	float u0 = (float)x0;
	float v0 = (float)y0;
	float u1 = (float)x1;
	float v1 = (float)y1;

	// Create quad vertices (unit square 0-1, will be scaled by xy in shader)
	vec2 positions[4] = {
		vec2(0.0f, 1.0f),  // Bottom-left
		vec2(0.0f, 0.0f),  // Top-left
		vec2(1.0f, 0.0f),  // Top-right
		vec2(1.0f, 1.0f)   // Bottom-right
	};

	// Texture coordinates
	vec2 texcoords[4] = {
		vec2(u0, v0),
		vec2(u0, v1),
		vec2(u1, v1),
		vec2(u1, v0)
	};

	// Indices for two triangles (quad)
	unsigned short indices[6] = { 0, 1, 2, 2, 3, 0 };

	// Create Metal buffers
	metal_mesh mesh;
	mesh.vertex_buffer = [_device newBufferWithBytes:positions
	                                          length:sizeof(positions)
	                                         options:MTLResourceStorageModeShared];
	mesh.texcoord_buffer = [_device newBufferWithBytes:texcoords
	                                            length:sizeof(texcoords)
	                                           options:MTLResourceStorageModeShared];
	mesh.index_buffer = [_device newBufferWithBytes:indices
	                                         length:sizeof(indices)
	                                        options:MTLResourceStorageModeShared];
	mesh.index_count = 6;
	mesh.image_id = image_id;
	mesh.xy = vec4(from_x, from_y, to_x, to_y);  // Pixel dimensions
	mesh.uv = vec4(u0, v0, u1, v1);               // Normalized texture coords
	mesh.is_sprite = true;

	// Store mesh
	meshes[key] = mesh;

	return key;
}

void xs::render::sprite(
	int sprite_id,
	double x,
	double y,
	double z,
	double scale,
	double rotation,
	color multiply,
	color add,
	unsigned int flags)
{
	if (!tools::check_bit_flag_overlap(flags, sprite_flags::fixed)) {
		x += offset.x;
		y += offset.y;
	}
	
	//x = round(x);
	//y = round(y);

	sprite_queue.push_back({
		sprite_id,
		x,
		y,
		z,
		scale,
		rotation,
		multiply,
		add,
		flags
	});
}

xs::render::stats xs::render::get_stats()
{
	return {};
}

void* xs::render::get_render_target_texture()
{
	return (void*)(intptr_t)_currentRenderTarget;
}
