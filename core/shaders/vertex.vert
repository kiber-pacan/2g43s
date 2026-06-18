#version 460

#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : enable

struct ModelBufferObject{
    mat4 model;
};

struct VisibleIndicesBufferObject {
    uint index;
};

struct TextureIndexBufferObject {
    uint firstIndex;
    uint indexCount;
    uint globalVertexOffset;
    uint pad2;
};

struct TextureIndexOffsetBufferObject {
    uint indexOffset;
};

layout(scalar, buffer_reference) readonly buffer UniformBufferObject {
    mat4 view; // Camera view
    mat4 projection; // Camera projection
    uint modelCount;
};

layout(scalar, buffer_reference) readonly buffer ModelBuffer{
    ModelBufferObject objects[];
};

layout(scalar, buffer_reference) readonly buffer VisibleIndicesBuffer {
    VisibleIndicesBufferObject objects[];
};


layout(scalar, buffer_reference) readonly buffer TextureIndexBuffer {
    TextureIndexBufferObject objects[];
};

layout(scalar, buffer_reference) readonly buffer TextureIndexOffsetBuffer {
    TextureIndexOffsetBufferObject objects[];
};

layout(push_constant) uniform Push {
    UniformBufferObject uniformBufferObject;
    ModelBuffer modelBuffer;
    VisibleIndicesBuffer visibleIndicesBuffer;
    TextureIndexBuffer textureIndexBuffer;
    TextureIndexOffsetBuffer textureIndexOffsetBuffer;
} constants;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out flat uint textureIndex;

void main() {
    uint modelIndex = gl_DrawID;
    uint instanceIndex = gl_InstanceIndex;

    uint visibleInstanceIndex = constants.visibleIndicesBuffer.objects[instanceIndex].index;
    gl_Position = constants.uniformBufferObject.projection * constants.uniformBufferObject.view * constants.modelBuffer.objects[visibleInstanceIndex].model * vec4(inPosition, 1.0);


    // Texture indexing stuff
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    uint firstIndex = constants.textureIndexBuffer.objects[modelIndex].firstIndex;
    uint rawIndex = 0;
    uint models = constants.uniformBufferObject.modelCount;

    uint textures = constants.textureIndexBuffer.objects[modelIndex].indexCount;

    for(uint i = 0; i < textures; i++) {
        if (gl_VertexIndex > constants.textureIndexBuffer.objects[modelIndex].globalVertexOffset + constants.textureIndexOffsetBuffer.objects[firstIndex + i].indexOffset) {
            rawIndex++;
        }
    }

    // This probably gonna get me brutally mauled one day
    if (rawIndex == 0) rawIndex = 1;

    //textureIndex = firstIndex + rawIndex;
    textureIndex = firstIndex + rawIndex;
}