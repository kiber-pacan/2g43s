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
const float resolution = 1024.0;

void main() {
    vec4 color = texture(texSampler, fragTexCoord);
    float pulse = sin(pc.upb.time * 2.0) * 0.5 + 0.5;
    outColor = mix(color, vec4(1.0 - color.rgb, 1.0), pulse * 0.8);
}