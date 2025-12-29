#ifndef KEYLISTENER_H
#define KEYLISTENER_H

#include <vector>
#include <SDL3/SDL_events.h>
#include "KeyBinding.hpp"
#include "../../Engine.hpp"
#include "cmath"

struct KeyListener {
    // All the listening keys
    KeyListener() {
        keys = {
            KeyBinding::of(SDL_SCANCODE_W),
            KeyBinding::of(SDL_SCANCODE_S),
            KeyBinding::of(SDL_SCANCODE_A),
            KeyBinding::of(SDL_SCANCODE_D),
            KeyBinding::of(SDL_SCANCODE_SPACE),
            KeyBinding::of(SDL_SCANCODE_LCTRL),
            KeyBinding::of(SDL_SCANCODE_LSHIFT),
            KeyBinding::of(SDL_SCANCODE_ESCAPE),
            KeyBinding::of(SDL_SCANCODE_R),
            KeyBinding::of(SDL_SCANCODE_MINUS),
            KeyBinding::of(SDL_SCANCODE_EQUALS),
            KeyBinding::of(SDL_SCANCODE_F2),
        };
    }

    std::vector<KeyBinding*> keys;


    void listen(const SDL_Event *e) const {
        for (KeyBinding* key : keys) {
            if (e->type == SDL_EVENT_KEY_UP && e->key.scancode == key->code) {
                key->down = false;
            } else if (e->type == SDL_EVENT_KEY_DOWN && e->key.scancode == key->code) {
                key->down  = true;
            }
        }
    }

    void iterateKeys(Engine& eng, bool& quit) const {
        // Movement
        if (keys[0]->down) {
            eng.camera.pos += eng.camera.speed * eng.camera.look * static_cast<float>(eng.deltaT->d);
        }
        if (keys[1]->down) {
            eng.camera.pos -= eng.camera.speed * eng.camera.look * static_cast<float>(eng.deltaT->d);
        }
        if (keys[2]->down) {
            auto cos = glm::cos(glm::radians(90.0f));
            auto sin = glm::sin(glm::radians(90.0f));

            glm::vec2 vec = glm::normalize(
                glm::vec2(
                    cos * eng.camera.look.x - sin * eng.camera.look.y,
                    sin * eng.camera.look.x + cos * eng.camera.look.y
                    )
                );

            eng.camera.pos.x += vec.x * eng.camera.speed * eng.deltaT->d;
            eng.camera.pos.y += vec.y * eng.camera.speed * eng.deltaT->d;
        }
        if (keys[3]->down) {
            auto cos = glm::cos(glm::radians(90.0f));
            auto sin = glm::sin(glm::radians(90.0f));

            glm::vec2 vec = glm::normalize(
                glm::vec2(
                    cos * eng.camera.look.x - sin * eng.camera.look.y,
                    sin * eng.camera.look.x + cos * eng.camera.look.y
                    )
                );


            eng.camera.pos.x -= vec.x * eng.camera.speed * eng.deltaT->d;
            eng.camera.pos.y -= vec.y * eng.camera.speed * eng.deltaT->d;
        }
        if (keys[4]->down) {
            eng.camera.pos.z += eng.camera.speed * eng.deltaT->d;
        }
        if (keys[5]->down) {
            eng.camera.pos.z -= eng.camera.speed * eng.deltaT->d;
        }
        if (keys[6]->down) {
            if (!eng.camera.running) {
                eng.camera.running = true;
                eng.camera.speed *= eng.camera.bonus;
            }
        } else if (!keys[6]->down) {
            if (eng.camera.running) {
                eng.camera.running = false;
                eng.camera.speed /= eng.camera.bonus;
            }
        }
        if (keys[7]->down) {
            quit = true;
        }
        if (keys[8]->down) {
            eng.camera.respawn();
        }
        if (keys[9]->down) {
            eng.camera.speed -= 50.0f;
            if (eng.camera.speed < 1.0f) eng.camera.speed = 1.0f;
        }
        if (keys[10]->down) {
            eng.camera.speed += 50.0f;
            if (eng.camera.speed > 100.0f) eng.camera.speed = 512.0f;
        }
        static bool wasDown = false;
        if (keys[11]->down && wasDown != true) {
            wasDown = true;
            SDL_SetWindowRelativeMouseMode(eng.window, !SDL_GetWindowRelativeMouseMode(eng.window));
        } else if (!keys[11]->down && wasDown == true) {
            wasDown = false;
        }
    }
};

#endif //KEYLISTENER_H
