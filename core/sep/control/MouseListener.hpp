#ifndef MOUSELISTENER_H
#define MOUSELISTENER_H

#include <vector>
#include <SDL3/SDL_events.h>
#include "KeyBinding.hpp"
#include "../../Engine.hpp"
#include "cmath"

struct MouseListener {
    static void listen(SDL_Event *e, SDL_Window* win, Camera& camera) {
        float x, y;

        SDL_GetRelativeMouseState(&x, &y);
        moveCamera(x, y, camera);
    }

    static void moveCamera(float x, float y, Camera& camera) {
        float& pitch = camera.pitch;
        float& yaw = camera.yaw;

        yaw -= x * camera.sens;
        pitch -= y * camera.sens;

        if (pitch > 89.0f) {
            pitch = 89.0f;
        }
        else if (pitch < -89.0f) {
            pitch = -89.0f;
        }

        glm::vec3 direction;

        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.z = sin(glm::radians(pitch));

        camera.look = glm::normalize(direction);
    }
};

#endif //MOUSELISTENER_H
