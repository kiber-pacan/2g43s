#version 450

layout(binding = 0) uniform sampler2D texSampler;

layout(binding = 1) uniform sampler2D depthSampler;

layout(binding = 2) uniform UPB {
    float time;
} upbo;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


const float NEAR = 0.1;
const float FAR = 4096.0;
const float resolution = 1024.0;

void main() {
    vec4 color = texture(texSampler, fragTexCoord);
    float pulse = sin(upbo.time * 2.0) * 0.5 + 0.5;
    outColor = mix(color, vec4(1.0 - color.rgb, 1.0), pulse * 0.8);
}