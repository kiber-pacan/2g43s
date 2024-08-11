#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <vulkan/vulkan.h>
#include "core/engine.hpp"
#include "core/logger.hpp"
#include "core/tools.hpp"

struct AppContext {
    SDL_Window* window{};
    engine vulkan_engine;
    SDL_bool app_quit = SDL_FALSE;
};


logger* LOGGER = logger::of("MAIN");

int SDL_Fail(){
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return -1;
}

int SDL_AppInit(void** appstate, int argc, char* argv[]) {
    // init the library, here we make a window so we only need the Video capabilities.
    if (SDL_Init(SDL_INIT_VIDEO)){
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

    auto* app = (AppContext*)appstate;

    LOGGER->log(logger::severity::LOG,"Session ID: $", vulkan_engine.sid);
    LOGGER->log(logger::severity::SUCCESS,"Application started successfully!", nullptr);

    /*
    LOGGER->log(logger::severity::LOG,"Hello $", "world");
    LOGGER->log(logger::severity::WARNING,"Hello $", "world");
    LOGGER->log(logger::severity::ERROR,"Hello $", "world");
    */

    return 0;
}

int SDL_AppEvent(void *appstate, const SDL_Event* event) {
    auto* app = (AppContext*)appstate;

    if (event->type == SDL_EVENT_QUIT) {
        app->app_quit = SDL_TRUE;
    }

    return 0;
}

int SDL_AppIterate(void *appstate) {
    auto* app = (AppContext*)appstate;
    engine& vulkan_engine = app->vulkan_engine;

    vulkan_engine.drawFrame();
    vkDeviceWaitIdle(vulkan_engine.device);

    return app->app_quit;
}

void SDL_AppQuit(void* appstate) {
    auto* app = (AppContext*)appstate;
    if (app) {
        SDL_DestroyWindow(app->window);
        delete app;
    }

    app->vulkan_engine.cleanup();
    SDL_Quit();
    LOGGER->log(logger::severity::SUCCESS,"Application quit successfully!", nullptr);
}
