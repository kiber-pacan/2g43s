#ifndef CAMERA_H
#define CAMERA_H
#include "../entity/CameraEntity.hpp"

struct KeyBinding;
class Engine;

struct Camera final : CameraEntity {
    Camera(const float fov, const float sens, const float speed, const float bonus, const glm::vec3 pos) {
        this->fov = fov;
        this->sens = sens;
        this->speed = speed;
        this->bonus = bonus;
        this->pos = pos;
    }

    Camera() {
        fov = 90.0f;
        sens = 0.42f;
        speed = 15.7f;
        bonus = 10.62f;

        pos = glm::vec3(-16, 0, 0);
        look = glm::vec3(1, 0, 0);
    }

    void respawn() override {
        pos = glm::vec3(-16, 0, 0), look = glm::vec3(1, 0, 0);
        yaw = 0, pitch = 0;
    }

    float yaw = 0, pitch = 0;
    float zNear{}, zFar{};
    float fov{}, sens{}, speed{}, bonus{};
    bool running = false;
};

#endif //CAMERA_H
