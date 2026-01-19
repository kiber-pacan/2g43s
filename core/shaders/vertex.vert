#version 460

#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : enable

struct MBO{
    mat4 model;
};

struct VIBO {
    uint index;
};

struct TIO {
    uint firstIndex;
    uint indexCount;
    uint pad1;
    uint pad2;
};

struct TIOO {
    uint indexOffset;
};

layout(scalar, buffer_reference) readonly buffer UBO {
    mat4 view;
    mat4 proj;
    uint modelCount;
};

layout(scalar, buffer_reference) readonly buffer MB{
    MBO objects[];
};

layout(scalar, buffer_reference) readonly buffer VIB {
    VIBO objects[];
};


layout(scalar, buffer_reference) readonly buffer TI {
    TIO objects[];
};

layout(scalar, buffer_reference) readonly buffer TIO1 {
    TIOO objects[];
};

layout(push_constant) uniform Push {
    UBO ubo;
    MB mb;
    VIB vib;
    TI ti;
    TIO1 tio;
} pc;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out flat uint textureIndex;

void main() {
    uint objIndex = pc.vib.objects[gl_InstanceIndex].index;
    gl_Position = pc.ubo.proj * pc.ubo.view * pc.mb.objects[objIndex].model * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;

    uint firstIndex = pc.ti.objects[gl_DrawID].firstIndex;
    uint rawIndex = 0;
    uint models = pc.ubo.modelCount;

    uint textures = pc.ti.objects[gl_DrawID].indexCount;
    for(uint i = 0; i < textures; i++) {
        if (gl_VertexIndex > pc.ti.objects[gl_DrawID].pad1 + pc.tio.objects[firstIndex + i].indexOffset) {
            rawIndex++;
        }
    }

    // This probably gonna get me brutally mauled one day
    if (rawIndex == 0) rawIndex = 1;

    textureIndex = firstIndex + rawIndex;
}