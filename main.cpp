#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <cmath>
#include <vulkan/vulkan.h>
#include "core/engine.hpp"
#include "core/logger.hpp"
#include "core/tools.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/sep/control/keyListener.hpp"
#include "core/sep/camera/camListener.hpp"


struct AppContext {
    SDL_Window* win{};
    engine eng;
    bool quit = true;

    keyListener* kL;
    camListener* cL;
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
        vulkan_engine,
        false,
        new keyListener(),
        new camListener()
    };

    LOGGER->log(logger::severity::LOG,"Session ID: $", vulkan_engine.sid);
    LOGGER->log(logger::severity::SUCCESS,"Application started successfully!", nullptr);

    SDL_SetWindowRelativeMouseMode(window, true);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e) {
    auto* app = static_cast<AppContext *>(appstate);

    if (e->type == SDL_EVENT_QUIT) {
        app->quit = true;
        return SDL_APP_SUCCESS;
    }

    if(app->quit) {
        return SDL_APP_SUCCESS;
    }

    float x, y;

    if (e->type == SDL_EVENT_MOUSE_MOTION && SDL_GetMouseFocus() == app->win) {
        SDL_GetRelativeMouseState(&x,&y);
        app->cL->moveCamera(x, y, app->eng.camera);
    }

    app->kL->listen(e);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    auto* app = static_cast<AppContext *>(appstate);
    engine& eng = app->eng;

    app->eng.deltaT->calculateDelta();
    app->kL->iterateCamMovement(app->quit, app->eng);

    eng.drawFrame();
    vkDeviceWaitIdle(eng.device);

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
