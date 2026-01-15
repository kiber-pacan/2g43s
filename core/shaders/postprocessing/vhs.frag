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
    vec2 uv = fragTexCoord;
    float jitter = sin(upbo.time * 50.0) * cos(upbo.time * 30.0) * 0.01;
    if (abs(sin(uv.y * 100.0)) > 0.95) uv.x += jitter;

    vec3 color = texture(texSampler, uv).rgb;
    float scanline = sin(uv.y * 800.0) * 0.04;
    outColor = vec4(color - scanline, 1.0);
}