#version 450
#extension GL_EXT_nonuniform_qualifier : enable



layout(binding = 1) uniform sampler2D texSampler[];



layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint textureIndex;

layout(location = 0) out vec4 outColor;

float near = 0.1;
float far = 100.0;

float linearizeDepth(float d) {
    return (near * far) / (far + d * (near - far));
}


void main() {
    outColor = texture(texSampler[nonuniformEXT(textureIndex)], fragTexCoord);
}