#version 460

#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : enable

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2D depthSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(scalar, buffer_reference) buffer UPB {
    vec2 resolution;
    float time;
};

layout(push_constant) uniform Push {
    UPB upb;
} pc;

const float NEAR = 0.1;
const float FAR = 4096.0;

float linearizeDepth(float d) {
    return (NEAR * FAR) / (FAR + d * (NEAR - FAR));
}


void main() {
    outColor = texture(texSampler, fragTexCoord);
}