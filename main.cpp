#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <cmath>
#include <vulkan/vulkan.h>
#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "core/Tools.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/sep/control/KeyListener.hpp"
#include "core/sep/control/MouseListener.hpp"

struct AppContext {
    SDL_Window* win{};
    Engine eng;
    bool quit = true;

    KeyListener* kL;
    MouseListener* mL;
};

Logger* LOGGER = Logger::of("main.cpp");

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
        LOGGER->info("Window size: ${} x ${}", width, height);
        LOGGER->info("Backbuffer size: ${} x ${}", bbwidth, bbheight);
        if (width != bbwidth){
            LOGGER->info("This is a highdpi environment.");
        }
        LOGGER->info("Random number: ${} (between 1 and 100)", Tools::randomNum<uint32_t>(1,100));
    }

    //Set up Vulkan engine
    Engine vulkan_engine;
    vulkan_engine.init(window);

    // set up the application data
    *appstate = new AppContext{
        window,
        vulkan_engine,
        false,
        new KeyListener(),
        new MouseListener()
    };

    LOGGER->info("Session ID: ${}", vulkan_engine.sid);
    LOGGER->success("Application started successfully!");

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

    app->kL->listen(e, app->eng, app->quit);
    app->mL->listen(e, app->win, app->eng.camera);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    auto* app = static_cast<AppContext *>(appstate);
    Engine& eng = app->eng;

    app->eng.deltaT->calculateDelta();
    app->kL->iterateKeys(eng, app->quit);

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
    LOGGER->success("Application quit successfully!");
}
