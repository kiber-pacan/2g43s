#version 460

struct MBO{
    mat4 model;
};

struct VIBO {
    uint index;
};

layout(binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(std430, binding = 2) readonly buffer MB{
    MBO objects[];
} mb;

layout(std430, binding = 3) readonly buffer VIB {
    VIBO objects[];
} vib;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    uint objIndex = vib.objects[gl_InstanceIndex].index;
    gl_Position = ubo.proj * ubo.view * mb.objects[objIndex].model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}