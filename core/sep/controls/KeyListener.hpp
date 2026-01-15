#ifndef KEYLISTENER_H
#define KEYLISTENER_H

#include <ranges>
#include <vector>
#include <SDL3/SDL_events.h>
#include "KeyBinding.hpp"
#include "../../Engine.hpp"
#include "cmath"

struct KeyListener {
    std::unordered_map<SDL_Scancode, KeyBinding> keyMap{};

    // All the listening keys
    KeyListener() {
        // Forward
        keyMap.emplace(SDL_SCANCODE_W, KeyBinding(
        [](Engine &engine) {
            if (!SDL_GetWindowRelativeMouseMode(engine.window)) return;
            engine.camera.pos += engine.camera.speed * engine.camera.look * (float) engine.delta.deltaTime;
        }));

        // Backward
        keyMap.emplace(SDL_SCANCODE_S, KeyBinding(
        [](Engine &engine) {
            if (!SDL_GetWindowRelativeMouseMode(engine.window)) return;
            engine.camera.pos -= engine.camera.speed * engine.camera.look * (float) engine.delta.deltaTime;
        }));

        // Strafe right
        keyMap.emplace(SDL_SCANCODE_A, KeyBinding([](Engine &engine) {
            if (!SDL_GetWindowRelativeMouseMode(engine.window)) return;
            glm::vec3 right = glm::normalize(glm::cross(engine.camera.look, glm::vec3(0.0f, 0.0f, 1.0f)));
            engine.camera.pos -= right * engine.camera.speed * (float) engine.delta.deltaTime;
        }));

        // Strafe left
        keyMap.emplace(SDL_SCANCODE_D, KeyBinding([](Engine &engine) {
            if (!SDL_GetWindowRelativeMouseMode(engine.window)) return;
            glm::vec3 right = glm::normalize(glm::cross(engine.camera.look, glm::vec3(0.0f, 0.0f, 1.0f)));
            engine.camera.pos += right * engine.camera.speed * (float) engine.delta.deltaTime;
        }));

        // Up
        keyMap.emplace(SDL_SCANCODE_SPACE, KeyBinding(
        [](Engine &engine) {
            if (!SDL_GetWindowRelativeMouseMode(engine.window)) return;
            engine.camera.pos.z += engine.camera.speed * (float) engine.delta.deltaTime;
        }));

        // Down
        keyMap.emplace(SDL_SCANCODE_LCTRL, KeyBinding(
        [](Engine &engine) {
            if (!SDL_GetWindowRelativeMouseMode(engine.window)) return;
            engine.camera.pos.z -= engine.camera.speed * (float) engine.delta.deltaTime;
        }));

        // Run
        keyMap.emplace(SDL_SCANCODE_LSHIFT, KeyBinding(
        [](Engine &eng) {
            if (!eng.camera.running) {
                eng.camera.running = true;
                eng.camera.speed *= eng.camera.bonus;
            }
        },
        nullptr,
        [](Engine &eng) {
            if (eng.camera.running) {
                eng.camera.running = false;
                eng.camera.speed /= eng.camera.bonus;
            }
        }));

        // Respawn
        keyMap.emplace(SDL_SCANCODE_R, KeyBinding(
        [](Engine &engine) {
            engine.camera.respawn();
        }));

        // Escape mouse
        keyMap.emplace(SDL_SCANCODE_F2, KeyBinding(nullptr,
        [](Engine &engine) {
            bool enabled = SDL_GetWindowRelativeMouseMode(engine.window);

            glm::vec2 pos;
            SDL_GetGlobalMouseState(&pos.x, &pos.y);

            if (!enabled) {
                engine.mousePointerPosition = pos;
                SDL_WarpMouseGlobal(engine.mousePosition.x, engine.mousePosition.y);
            } else {
                engine.mousePosition = pos;
                SDL_WarpMouseGlobal(engine.mousePointerPosition.x, engine.mousePointerPosition.y);
            }

            SDL_SetWindowRelativeMouseMode(engine.window, !enabled);
        }));

        // Increase speed
        keyMap.emplace(SDL_SCANCODE_EQUALS, KeyBinding(nullptr,
        [](Engine &engine) {
            engine.camera.speed += 50.0f;
            if (engine.camera.speed > 100.0f) engine.camera.speed = 512.0f;
        }));

        // Decrease speed
        keyMap.emplace(SDL_SCANCODE_MINUS, KeyBinding(nullptr,
        [](Engine &engine) {
            engine.camera.speed -= 50.0f;
            if (engine.camera.speed < 1.0f) engine.camera.speed = 1.0f;
        }));

        // Reload shaders
        keyMap.emplace(SDL_SCANCODE_F3, KeyBinding(
        [](Engine &engine) {
            std::vector<std::filesystem::path> dirtyShaders = Shaders::getShadersToCompile();
            for (auto& dirtyShader : dirtyShaders) {
                auto shader = Shaders::compileShader(dirtyShader);
                vkDeviceWaitIdle(engine.device);
                Shaders::saveShaderToFile(Tools::getShaderPath() + "compiled/" + dirtyShader.lexically_relative(Tools::getShaderPath()).replace_extension(".spv").string(), shader);
                if (dirtyShader.string().contains("postprocessing")) engine.recreatePostprocessingPipeline(dirtyShader.filename().replace_extension(".spv"));
            }
        }));
    }

    void listen(SDL_Event *event) {
        auto it = keyMap.find(event->key.scancode);

        if (it != keyMap.end()) {
            auto& key = it->second;
            if (event->type == SDL_EVENT_KEY_UP) {
                key.down = false;
                key.released = true;
            } else if (event->type == SDL_EVENT_KEY_DOWN) {
                key.down = true;
                key.pressed = true;
            }
        }
    }

    void iterateKeys(Engine& engine) {
        for (auto &key: keyMap | std::views::values) {
            if (key.down && key.onDown != nullptr) key.onDown(engine);
            if (key.pressed && key.onPress != nullptr) key.onPress(engine); key.pressed = false;
            if (key.released && key.onRelease != nullptr) key.onRelease(engine); key.released = false;
        }
    }
};

#endif //KEYLISTENER_H
