#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <vulkan/vulkan.h>
#include "gamecore/engine.hpp"
#include "gamecore/logger.cpp"
#include "gamecore/tools.hpp"

struct AppContext {
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_bool app_quit = SDL_FALSE;
    engine vulkan_engine{};
};

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
    SDL_Window* window = SDL_CreateWindow("Window", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window){
        return SDL_Fail();
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer){
        return SDL_Fail();
    }


    // print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        SDL_Log("Window size: %ix%i", width, height);
        SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
        if (width != bbwidth){
            SDL_Log("This is a highdpi environment.");
        }
        SDL_Log("Random number: %i (between 1 and 100)", tools::randomNum<uint32_t>(1,100));
    }

    // set up the application data
    *appstate = new AppContext{
       window,
       renderer,
    };

    auto* app = (AppContext*)appstate;

    //Set up Vulkan engine
    app->vulkan_engine.init();
    SDL_Log("Session ID: %i", app->vulkan_engine.sid);

    SDL_Log("Application started successfully!");

    logger* LOGGER = logger::of("dodk", 35);
    const char* str = "devihl";
    LOGGER->log("daun $ $ $", 2, 1, 5);

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

    // draw a color
    auto time = SDL_GetTicks() / 1000.f;
    auto red = (std::sin(time) + 1) / 2.0 * 255;
    auto green = (std::sin(time / 2) + 1) / 2.0 * 255;
    auto blue = (std::sin(time) * 2 + 1) / 2.0 * 255;

    SDL_SetRenderDrawColor(app->renderer, red, green, blue, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(app->renderer);
    SDL_RenderPresent(app->renderer);

    return app->app_quit;
}

void SDL_AppQuit(void* appstate) {
    auto* app = (AppContext*)appstate;
    if (app) {
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        delete app;
    }

    SDL_Quit();
    SDL_Log("Application quit successfully!");
}