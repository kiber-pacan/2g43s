#ifndef KEY_H
#define KEY_H
#include <SDL3/SDL_scancode.h>
#include "Engine.hpp"
#include <functional>

struct KeyBinding {
    explicit KeyBinding(const std::function<void(Engine& eng)>& onDown) {
        this->onDown = onDown;
    }

    explicit KeyBinding(const std::function<void(Engine& eng)>& onDown, const std::function<void(Engine& eng)>& onRelease) {
        this->onDown = onDown;
        this->onRelease = onRelease;
    }

    explicit KeyBinding(const std::function<void(Engine& eng)>& onDown, const std::function<void(Engine& eng)>& onPress, const std::function<void(Engine& eng)>& onRelease) {
        this->onDown = onDown;
        this->onPress = onPress;
        this->onRelease = onRelease;
    }

    std::function<void(Engine& eng)> onDown;
    std::function<void(Engine& eng)> onPress;
    std::function<void(Engine& eng)> onRelease;

    bool down = false;
    bool pressed = false;
    bool released = false;
};

#endif //KEY_H
