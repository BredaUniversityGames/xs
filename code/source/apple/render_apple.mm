#include "render.h"
#include "configuration.h"
#include "profiler.h"

#import <MetalKit/MetalKit.h>

#import <simd/simd.h>
#import <ModelIO/ModelIO.h>

static const NSUInteger MaxBuffersInFlight = 3;

namespace xs::render::internal
{
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
}

using namespace xs;
using namespace xs::render::internal;

void xs::render::initialize()
{
}

void xs::render::shutdown()
{
}

void xs::render::render()
{
    XS_PROFILE_SECTION("xs::render::render");
}

void xs::render::clear()
{
}

/*
void xs::render::internal::create_texture_with_data(xs::render::internal::image& img, uchar* data)
{
}
 */

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
