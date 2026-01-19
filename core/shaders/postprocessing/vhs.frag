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
    float jitter = sin(pc.upb.time * 50.0) * cos(pc.upb.time * 30.0) * 0.01;
    if (abs(sin(uv.y * 100.0)) > 0.95) uv.x += jitter;

    vec3 color = texture(texSampler, uv).rgb;
    float scanline = sin(uv.y * 800.0) * 0.04;
    outColor = vec4(color - scanline, 1.0);
}