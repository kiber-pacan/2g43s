#version 450

layout(binding = 1) uniform sampler2DArray texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


float near = -1.0;
float far = 1000.0;

float linearizeDepth(float d) {
    return (near * far) / (far + d * (near - far));
}


void main() {
    float depth = texture(texSampler, vec3(fragTexCoord, 0)).r;

    float linearD = linearizeDepth(depth) / far;

    outColor = vec4(vec3(linearD), 1.0);
}