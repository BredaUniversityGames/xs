#include "render.h"
#include "render_apple.h"
#include "render_internal.h"
#include "configuration.h"
#include "profiler.h"
#include "device_apple.h"
#include "device.h"
#include "tools.h"
#include "log.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#import <MetalKit/MetalKit.h>
#import <simd/simd.h>
#import <ModelIO/ModelIO.h>
#import "shader_types.h"

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

    // Texture to render to and then sample from.
    id<MTLTexture> _renderTargetTexture;

    // Render pass descriptor to draw to the texture
    MTLRenderPassDescriptor* _renderToTextureRenderPassDescriptor;

    // A pipeline object to render to the offscreen texture.
    id<MTLRenderPipelineState> _renderToTextureRenderPipeline;

    id<MTLRenderPipelineState> _debugRenderPipeline;

    MTLRenderPipelineDescriptor* _pipelineStateDescriptor;

    int const                sprite_trigs_max = 21800;
    int                      sprite_trigs_count = 0;
    sprite_vtx_format        sprite_trigs_array[sprite_trigs_max * 3];
    void render_to_view();


	// TODO: Deprecated
	struct sprite_data
	{
		int image_id	= -1;
		glm::vec2 from	= {};
		glm::vec2 to	= {};
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

	std::vector<sprite_data>				sprites;
	std::vector<sprite_queue_entry>	sprite_queue = {};

}

void xs::render::initialize()
{
    NSError *error;
    
    MTKView* view = device::internal::get_view();
    _device = view.device;
    
    // Save the size of the drawable to pass to the vertex shader.
    _viewportSize.x = configuration::width();
    _viewportSize.y = configuration::height();
    
    // Load all the shader files with a .metal file extension in the project.
    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];

    // Render to texture
    {
        NSError *error;
        
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertex_shader"];
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragment_shader"];
				
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
        
        _renderTargetTexture = [_device newTextureWithDescriptor:texDescriptor];
        assert(_renderTargetTexture);

        _renderToTextureRenderPassDescriptor = [MTLRenderPassDescriptor new];
        _renderToTextureRenderPassDescriptor.colorAttachments[0].texture = _renderTargetTexture;
        _renderToTextureRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        _renderToTextureRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
        _renderToTextureRenderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        
        MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineStateDescriptor.label = @"xs render to texture pipeline";
        pipelineStateDescriptor.rasterSampleCount = 1;
        pipelineStateDescriptor.vertexFunction = vertexFunction;
        pipelineStateDescriptor.fragmentFunction = fragmentFunction;
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = _renderTargetTexture.pixelFormat;
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
        pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        
        _renderToTextureRenderPipeline = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
        assert(_renderToTextureRenderPipeline);
        
        id<MTLFunction> vertexDebugFunction = [defaultLibrary newFunctionWithName:@"vertex_shader_debug"];
        id<MTLFunction> fragmentDebugFunction = [defaultLibrary newFunctionWithName:@"fragment_shader_debug"];
                
        MTLRenderPipelineDescriptor *debugStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        debugStateDescriptor.label = @"xs debug render pipeline";
        debugStateDescriptor.rasterSampleCount = 1;
        debugStateDescriptor.vertexFunction = vertexDebugFunction;
        debugStateDescriptor.fragmentFunction = fragmentDebugFunction;
        debugStateDescriptor.colorAttachments[0].pixelFormat = _renderTargetTexture.pixelFormat;
        debugStateDescriptor.colorAttachments[0].blendingEnabled = YES;
        debugStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        debugStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        debugStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        debugStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        
        _debugRenderPipeline = [_device newRenderPipelineStateWithDescriptor:debugStateDescriptor error:&error];
        assert(_debugRenderPipeline);
    }
    
    // Render to view
    {
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertex_shader_screen"];
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragment_shader_screen"];

        _pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        _pipelineStateDescriptor.label = @"xs render to screen pipeline";
        _pipelineStateDescriptor.vertexFunction = vertexFunction;
        _pipelineStateDescriptor.fragmentFunction = fragmentFunction;
        _pipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
        
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
    XS_PROFILE_SECTION("xs::render::render");
        
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
    [render_encoder setRenderPipelineState:_renderToTextureRenderPipeline];
    
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

            // Draw the triangles
            [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                vertexStart:0
                vertexCount:count];
            
            count = 0;
        }
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
}

void xs::render::composite()
{
    MTKView* view = device::internal::get_view();
    
    MTLRenderPassDescriptor *drawableRenderPassDescriptor = view.currentRenderPassDescriptor;
    if(drawableRenderPassDescriptor != nil)
    {
        const float dw = device::get_width();
        const float dh = device::get_height();
        const float cw = configuration::width();
        const float ch = configuration::height();
        const float aspect = (cw / ch) / (dw / dh);
        const float w = aspect;
        const float h = 1.0f;
        const screen_vtx_format quadVertices[] =
        {
            // Positions     , Texture coordinates
            { {  w,  -h },  { 1.0, 1.0 } },
            { { -w,  -h },  { 0.0, 1.0 } },
            { { -w,   h },  { 0.0, 0.0 } },
            
            { {  w,  -h },  { 1.0, 1.0 } },
            { { -w,   h },  { 0.0, 0.0 } },
            { {  w,   h },  { 1.0, 0.0 } },
        };
        
        id<MTLRenderCommandEncoder> renderEncoder = device::internal::get_render_encoder();
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        [renderEncoder setVertexBytes:&quadVertices
                               length:sizeof(quadVertices)
                              atIndex:index_vertices];
        
        // Set the offscreen texture as the source texture.
        [renderEncoder setFragmentTexture:_renderTargetTexture atIndex:index_sprite_texture];
        
        // Draw quad with rendered texture.
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                          vertexStart:0
                          vertexCount:6];
        
        // End encoding will be called by the device on the "default" encoder
    }
}

void xs::render::clear()
{
    sprite_queue.clear();
}

void xs::render::create_texture_with_data(
    xs::render::image& img,
    uchar* data)
{
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

// TODO: Deprecated
int xs::render::create_shape(
	int image_id,
	const float *positions,
	const float *texture_coordinates,
	unsigned int vertex_count,
	const unsigned short *indices,
	unsigned int index_count)
{
	return 0;
}

void xs::render::destroy_shape(int sprite_id) {}

int xs::render::create_sprite(int image_id, double x0, double y0, double x1, double y1)
{
	if (image_id < 0 || image_id >= images.size()) {
		log::error("Can't create sprite with image {}!", image_id);
		return -1;
	}

	const auto epsilon = 0.001;
	for(int i = 0; i < sprites.size(); i++)
	{
		const auto& s = sprites[i];
		if (s.image_id == image_id &&
			abs(s.from.x - x0) < epsilon &&
			abs(s.from.y - y0) < epsilon &&
			abs(s.to.x - x1) < epsilon &&
			abs(s.to.y - y1) < epsilon)
			return i;
	}

	const auto i = sprites.size();
	sprite_data s = { image_id, { x0, y0 }, { x1, y1 } };
	sprites.push_back(s);
	return static_cast<int>(i);
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
	if (sprite_id < 0 || sprite_id >= sprites.size()) {
		log::error("Can't render sprite {}!", sprite_id);
		return;
	}

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
