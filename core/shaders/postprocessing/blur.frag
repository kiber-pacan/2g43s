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
    vec2 dir = 0.5 - uv;
    float dist = length(dir);
    vec4 color = vec4(0.0);
    for(int i = 0; i < 10; i++) {
        color += texture(texSampler, uv + dir * float(i) * 0.02);
    }
    outColor = color / 10.0;
}