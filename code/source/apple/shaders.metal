

/*
 #include <metal_stdlib>
 #include <simd/simd.h>
 
 // Including header shared between this Metal shader code and Swift/C code executing Metal API commands
 #import "shader_types.h"
 
 using namespace metal;
 
 typedef struct
 {
 float3 position [[attribute(VertexAttributePosition)]];
 float2 texCoord [[attribute(VertexAttributeTexcoord)]];
 } Vertex;
 
 typedef struct
 {
 float4 position [[position]];
 float2 texCoord;
 } ColorInOut;
 
 vertex ColorInOut vertexShader(Vertex in [[stage_in]],
 constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]])
 {
 ColorInOut out;
 
 float4 position = float4(in.position, 1.0);
 out.position = uniforms.projectionMatrix * uniforms.modelViewMatrix * position;
 out.texCoord = in.texCoord;
 
 return out;
 }
 
 fragment float4 fragmentShader(ColorInOut in [[stage_in]],
 constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]],
 texture2d<half> colorMap     [[ texture(TextureIndexColor) ]])
 {
 constexpr sampler colorSampler(mip_filter::linear,
 mag_filter::linear,
 min_filter::linear);
 
 half4 colorSample   = colorMap.sample(colorSampler, in.texCoord.xy);
 
 return float4(colorSample);
 }
 */

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#import "shader_types.h"

struct RasterizerData
{
    // The [[position]] attribute of this member indicates that this value
    // is the clip space position of the vertex when this structure is
    // returned from the vertex function.
    float4 position [[position]];


    // Since this member does not have a special attribute, the rasterizer
    // interpolates its value with the values of the other triangle vertices
    // and then passes the interpolated value to the fragment shader for each
    // fragment in the triangle.
    float4 color;
};


vertex RasterizerData vertexShader(uint vertexID [[vertex_id]],
             constant AAPLVertex *vertices [[buffer(AAPLVertexInputIndexVertices)]],
             constant vector_uint2 *viewportSizePointer [[buffer(AAPLVertexInputIndexViewportSize)]])
{
    RasterizerData out;

    // Index into the array of positions to get the current vertex.
    // The positions are specified in pixel dimensions (i.e. a value of 100
    // is 100 pixels from the origin).
    float2 pixelSpacePosition = vertices[vertexID].position.xy;

    // Get the viewport size and cast to float.
    vector_float2 viewportSize = vector_float2(*viewportSizePointer);
    

    // To convert from positions in pixel space to positions in clip-space,
    //  divide the pixel coordinates by half the size of the viewport.
    out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
    out.position.xy = pixelSpacePosition / (viewportSize / 2.0);

    // Pass the input color directly to the rasterizer.
    out.color = vertices[vertexID].color;

    return out;
}


fragment float4 fragmentShader(RasterizerData in [[stage_in]])
{
    // Return the interpolated color.
    return in.color;
}

struct vertex_to_fragment
{
    vec4 position [[position]];
    vec4 mul_color;
    vec4 add_color;
    vec2 texture;
};

vertex vertex_to_fragment vertex_shader(
    uint vertexID [[vertex_id]],
    constant sprite_vtx_format* vertices [[buffer(index_vertices)]],
    constant mat4* p_worldviewproj [[buffer(index_wvp)]])
{
    mat4 worldviewproj = mat4(*p_worldviewproj);
    
    vertex_to_fragment vtf;
    vtf.position = vertices[vertexID].position * worldviewproj;
    vtf.texture = vertices[vertexID].texture;
    vtf.add_color = vertices[vertexID].add_color;
    vtf.mul_color = vertices[vertexID].mul_color;
    
    return vtf;
}

fragment float4 fragment_shader(vertex_to_fragment vtf [[stage_in]])
{
    return float4(1.0, 1.0, 1.0, 1.0);
}
