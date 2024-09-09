#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <vulkan/vulkan.h>
#include "core/engine.hpp"
#include "core/logger.hpp"
#include "core/tools.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct AppContext {
    SDL_Window* win{};
    engine eng;
    SDL_bool quit = SDL_FALSE;
};


logger* LOGGER = logger::of("MAIN");

SDL_AppResult SDL_Fail() {
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    // init the library, here we make a window so we only need the Video capabilities.
    if (!SDL_Init(SDL_INIT_VIDEO)){
        return SDL_Fail();
    }

    // create a window
    SDL_Window* window = SDL_CreateWindow("Window", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    if (!window){
        return SDL_Fail();
    }


    // print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        LOGGER->log(logger::severity::LOG,"Window size: $x$", width, height);
        LOGGER->log(logger::severity::LOG,"Backbuffer size: $x$", bbwidth, bbheight);
        if (width != bbwidth){
            LOGGER->log(logger::severity::LOG,"This is a highdpi environment.", nullptr);
        }
        LOGGER->log(logger::severity::LOG,"Random number: $ (between 1 and 100)", tools::randomNum<uint32_t>(1,100));
    }

    //Set up Vulkan engine
    engine vulkan_engine;
    vulkan_engine.init(window);

    // set up the application data
    *appstate = new AppContext{
        window,
        vulkan_engine
    };

    LOGGER->log(logger::severity::LOG,"Session ID: $", vulkan_engine.sid);
    LOGGER->log(logger::severity::SUCCESS,"Application started successfully!", nullptr);

    return SDL_APP_CONTINUE;
}

bool w = false;
bool a = false;
bool s = false;
bool d = false;


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e) {
    auto* app = static_cast<AppContext *>(appstate);

    if (e->type == SDL_EVENT_QUIT) {
        app->quit = SDL_TRUE;
        return SDL_APP_SUCCESS;
    }

    if (e->type == SDL_EVENT_KEY_UP && e->key.scancode == SDL_SCANCODE_W) {
        w = false;
    } else if (e->type == SDL_EVENT_KEY_DOWN && e->key.scancode == SDL_SCANCODE_W) {
        w = true;
    }
    if (e->type == SDL_EVENT_KEY_UP && e->key.scancode == SDL_SCANCODE_S) {
        s = false;
    } else if (e->type == SDL_EVENT_KEY_DOWN && e->key.scancode == SDL_SCANCODE_S) {
        s = true;
    }
    if (e->type == SDL_EVENT_KEY_UP && e->key.scancode == SDL_SCANCODE_A) {
        a = false;
    } else if (e->type == SDL_EVENT_KEY_DOWN && e->key.scancode == SDL_SCANCODE_A) {
        a = true;
    }
    if (e->type == SDL_EVENT_KEY_UP && e->key.scancode == SDL_SCANCODE_D) {
        d = false;
    } else if (e->type == SDL_EVENT_KEY_DOWN && e->key.scancode == SDL_SCANCODE_D) {
        d = true;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    auto* app = static_cast<AppContext *>(appstate);
    engine& vulkan_engine = app->eng;

    if (w) app->eng.pos.x += 0.1;
    if (s) app->eng.pos.x -= 0.1;
    if (d) app->eng.pos.z += 0.1;
    if (a) app->eng.pos.z -= 0.1;


    vulkan_engine.drawFrame();
    vkDeviceWaitIdle(vulkan_engine.device);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate) {
    // Cleanup
    if (auto* app = static_cast<AppContext *>(appstate)) {
        app->eng.cleanup();
        SDL_DestroyWindow(app->win);
        delete app;
    }

    SDL_Quit();
    LOGGER->log(logger::severity::SUCCESS,"Application quit successfully!", nullptr);
}
