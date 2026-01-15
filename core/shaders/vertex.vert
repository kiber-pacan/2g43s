#version 460

#extension GL_EXT_shader_draw_parameters : enable
#extension GL_EXT_debug_printf : enable

struct MBO{
    mat4 model;
};

struct VIBO {
    uint index;
};

layout(binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    uint modelCount;
} ubo;

layout(std430, binding = 2) readonly buffer MB{
    MBO objects[];
} mb;

layout(std430, binding = 3) readonly buffer VIB {
    VIBO objects[];
} vib;

struct TIO {
    uint firstIndex;
    uint indexCount;
    uint pad1;
    uint pad2;
};

struct TIOO {
    uint indexOffset;
};

layout(std430, binding = 4) readonly buffer TI {
    TIO objects[];
} ti;

layout(std430, binding = 5) readonly buffer TIO1 {
    TIOO objects[];
} tio;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out flat uint textureIndex;

void main() {
    uint objIndex = vib.objects[gl_InstanceIndex].index;
    gl_Position = ubo.proj * ubo.view * mb.objects[objIndex].model * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;

    uint firstIndex = ti.objects[gl_DrawID].firstIndex;
    uint rawIndex = 0;
    uint models = ubo.modelCount;

    uint textures = ti.objects[gl_DrawID].indexCount;
    for(uint i = 0; i < textures; i++) {
        if (gl_VertexIndex > ti.objects[gl_DrawID].pad1 + tio.objects[firstIndex + i].indexOffset) {
            rawIndex++;
        }
    }

    // This probably gonna get me brutally mauled one day
    if (rawIndex == 0) rawIndex = 1;

    textureIndex = firstIndex + rawIndex;
}