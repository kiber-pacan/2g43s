#version 450

layout(binding = 0) uniform sampler2D texSampler;

layout(binding = 1) uniform sampler2D depthSampler;

layout(binding = 2) uniform UPB {
    float time;
} upbo;


layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = fragTexCoord - 0.5;
    float dist = length(uv);
    float angle = atan(uv.y, uv.x) + sin(dist * 10.0 - upbo.time) * 2.0;
    vec2 newUV = vec2(cos(angle), sin(angle)) * dist + 0.5;
    outColor = texture(texSampler, newUV);
}