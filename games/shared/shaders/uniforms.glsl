#define MAX_INSTANCES 128
#define INSTANCES_UBO_LOCATION 1

struct instance_struct
{
    mat4    wvp;        // 64
    vec4    mul_color;  // 16   
    vec4    add_color;  // 16
    uint    flags;      // 4
};

layout(std140, binding = INSTANCES_UBO_LOCATION) uniform instances_ubo
{
    instance_struct instances[MAX_INSTANCES];
};
