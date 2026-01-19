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
    vec3 pix = texture(texSampler, fragTexCoord).rgb;
    float lum = dot(pix, vec3(0.299, 0.587, 0.114));
    vec3 thermal = vec3(lum < 0.5 ? 0.0 : (lum - 0.5) * 2.0, lum, 1.0 - lum);
    outColor = vec4(thermal, 1.0);
}