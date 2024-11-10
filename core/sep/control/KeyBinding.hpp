#ifndef KEY_H
#define KEY_H
#include <SDL3/SDL_scancode.h>

struct KeyBinding {
    static KeyBinding* of(SDL_Scancode key) {
        return new KeyBinding(key);
    }

    SDL_Scancode code;
    bool down = false;
private:
    explicit KeyBinding(SDL_Scancode key) {
        this->code = key;
    }
};

#endif //KEY_H
