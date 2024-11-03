#define MAX_INSTANCES 128
#define INSTANCES_UBO_LOCATION 1

struct instance_struct
{
    vec4 mul_color; // 16   
    vec4 add_color; // 16
    vec2 position;  // 8
    vec2 scale;	    // 8
    float rotation; // 4
    uint flags;     // 4
};

layout(std140, binding = INSTANCES_UBO_LOCATION) uniform instances_ubo
{
    instance_struct instances[MAX_INSTANCES];
};
