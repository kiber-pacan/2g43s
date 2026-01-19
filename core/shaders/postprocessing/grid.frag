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
    vec4 color = texture(texSampler, fragTexCoord);
    vec2 gridUV = fragTexCoord * 50.0;
    float line = step(0.95, fract(gridUV.x + pc.upb.time * 0.5)) + step(0.95, fract(gridUV.y));
    outColor = mix(color, vec4(0.0, 1.0, 0.0, 1.0), line * 0.3);
}