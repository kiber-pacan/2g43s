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

const float NEAR = 0.1;
const float FAR = 4096.0;

float linearizeDepth(float d) {
    return (NEAR * FAR) / (FAR + d * (NEAR - FAR));
}

const float exposure = 1.5;      // Общая яркость
const float bloomThreshold = 0.7; // С какого порога начинаем "светить"
const float bloomScale = 0.8;     // Мощность свечения
const float gamma = 2.2;          // Коррекция под монитор

void main() {
    vec2 uv = fragTexCoord;

    // 1. Читаем основной цвет (HDR)
    vec3 hdrColor = texture(texSampler, uv).rgb;

    // 2. ИМИТАЦИЯ БЛЮМА (Pseudo-Bloom)
    // В идеале делается в несколько пассов, но мы сделаем быстрый 5-tap для "мягкости"
    float offset = 0.003;
    vec3 blur = texture(texSampler, uv).rgb * 0.204;
    blur += texture(texSampler, uv + vec2(offset, 0)).rgb * 0.199;
    blur += texture(texSampler, uv - vec2(offset, 0)).rgb * 0.199;
    blur += texture(texSampler, uv + vec2(0, offset)).rgb * 0.199;
    blur += texture(texSampler, uv - vec2(0, offset)).rgb * 0.199;

    // Выделяем только яркие участки для свечения
    vec3 bloom = max(blur - vec3(bloomThreshold), 0.0);

    // 3. ПРИМЕНЯЕМ ЭКСПОЗИЦИЮ (как в HL2 при адаптации зрения)
    // Можно добавить пульсацию, если персонаж только что вышел из тени
    hdrColor *= exposure;
    bloom *= bloomScale;

    // 4. СМЕШИВАНИЕ (Add-blend)
    vec3 result = hdrColor + bloom;

    // 5. ТОНМАППИНГ (Reinhard)
    // Это превращает HDR-величины (которые > 1.0) в понятные монитору LDR (0..1)
    // Именно это дает "киношный" мягкий вид вместо вырвиглазных пикселей
    vec3 mapped = result / (result + vec3(1.0));

    // 6. ГАММА-КОРРЕКЦИЯ
    mapped = pow(mapped, vec3(1.0 / gamma));

    outColor = vec4(mapped, 1.0);
}