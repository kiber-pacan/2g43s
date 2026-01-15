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
    float finalResolution = clamp(abs(resolution * cos(upbo.time)), 32.0, 100000);
    vec2 gridUV = floor(fragTexCoord * finalResolution) / finalResolution;
    outColor = 1.0 - texture(texSampler, gridUV);
}