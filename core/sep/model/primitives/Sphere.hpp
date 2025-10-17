#ifndef INC_2G43S_SHERE_H
#define INC_2G43S_SHERE_H
#include <glm/vec4.hpp>

struct Sphere {
    glm::vec4 sphere;

    Sphere() = default;
    explicit Sphere(const glm::vec4& sphere) : sphere(sphere) {}
};

#endif //INC_2G43S_SHERE_H