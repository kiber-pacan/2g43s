#version 460

struct MBO{
    mat4 model;
};

layout(binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(std140, binding = 2) readonly buffer MB{
    MBO objects[];
} mb;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * mb.objects[gl_InstanceIndex].model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}