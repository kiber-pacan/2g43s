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

void main() {
    vec2 uv = fragTexCoord;
    uv.x += sin(uv.y * 10.0 + pc.upb.time) * 0.02;
    uv.y += cos(uv.x * 10.0 + pc.upb.time) * 0.02;
    outColor = texture(texSampler, uv);
}