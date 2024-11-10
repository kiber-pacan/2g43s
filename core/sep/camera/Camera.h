#ifndef CAMERA_H
#define CAMERA_H
#include "../entity/Entity.h"

struct KeyBinding;
class Engine;

struct Camera : Entity {
    Camera(float fov, float sens, float speed, float bonus, glm::vec3 pos) {
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
        bonus = 3.62f;

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
