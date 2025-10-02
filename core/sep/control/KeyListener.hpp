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
            KeyBinding::of(SDL_SCANCODE_R)
        };
    }

    std::vector<KeyBinding*> keys;


    void listen(SDL_Event *e, Engine& eng, bool& quit) {
        for (KeyBinding* key : keys) {
            if (e->type == SDL_EVENT_KEY_UP && e->key.scancode == key->code) {
                key->down = false;
            } else if (e->type == SDL_EVENT_KEY_DOWN && e->key.scancode == key->code) {
                key->down  = true;
            }
        }
    }

    void iterateKeys(Engine& eng, bool& quit) {
        // Movement
        if (keys[0]->down) {
            eng.camera.pos += eng.camera.speed * eng.camera.look * eng.deltaT->d;
        }
        if (keys[1]->down) {
            eng.camera.pos -= eng.camera.speed * eng.camera.look * eng.deltaT->d;
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
    }
};

#endif //KEYLISTENER_H
