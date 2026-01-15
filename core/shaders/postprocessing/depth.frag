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

float linearizeDepth(float d) {
    return (NEAR * FAR) / (FAR + d * (NEAR - FAR));
}


void main() {
    float rawDepth = texture(texSampler, fragTexCoord).r;

    float dist = linearizeDepth(rawDepth);

    float expColor = dist / 64.0;

    float final = 1.0 - clamp(expColor, 0.0, 1.0);

    outColor = vec4(vec3(final), 1.0);
}