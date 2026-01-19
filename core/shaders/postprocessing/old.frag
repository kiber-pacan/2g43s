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

float noise(vec2 co) { return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453); }
void main() {
    vec3 pix = texture(texSampler, fragTexCoord).rgb;
    vec3 sepia = vec3(dot(pix, vec3(0.393, 0.769, 0.189)),
    dot(pix, vec3(0.349, 0.686, 0.168)),
    dot(pix, vec3(0.272, 0.534, 0.131)));
    vec2 uv = fragTexCoord * pc.upb.time;
    float g = noise(uv) * 0.15;
    outColor = vec4(sepia + g, 1.0);
}