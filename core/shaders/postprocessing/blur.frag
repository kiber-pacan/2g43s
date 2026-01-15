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
    vec2 dir = 0.5 - uv;
    float dist = length(dir);
    vec4 color = vec4(0.0);
    for(int i = 0; i < 10; i++) {
        color += texture(texSampler, uv + dir * float(i) * 0.02);
    }
    outColor = color / 10.0;
}