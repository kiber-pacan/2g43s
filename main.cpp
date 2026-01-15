#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <cmath>
#include <vulkan/vulkan.h>
#include "core/Engine.hpp"
#include "core/sep/util/Logger.hpp"
#include "core/sep/util/Tools.hpp"

#define GLM_FORCE_RADIANS
#include "imgui_impl_sdl3.h"
#include <glm/glm.hpp>

#include "core/sep/controls/KeyListener.hpp"
#include "core/sep/controls/MouseListener.hpp"
#include "core/sep/util/Random.hpp"


#include <omp.h>
#include <thread>


struct AppContext {
    Engine engine;
    KeyListener* kL;
    MouseListener* mL;
};

Logger LOGGER = Logger("main.cpp");

SDL_AppResult SDL_Fail() {
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    #if defined(__linux__)
        setenv("OMP_PROC_BIND", "true", 1);
        setenv("OMP_PLACES", "cores", 1);
    #endif

    // Init the library, here we make a window so we only need the Video capabilities.
    if (!SDL_Init(SDL_INIT_VIDEO)){
        return SDL_Fail();
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Window", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    if (!window){
        return SDL_Fail();
    }


    // Print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        LOGGER.info("Window size: ${} x ${}", width, height);
        LOGGER.info("Backbuffer size: ${} x ${}", bbwidth, bbheight);
        if (width != bbwidth){
            LOGGER.info("This is a highdpi environment.");
        }
        LOGGER.info("Random number: ${} (between 1 and 100)", Random::randomNum<uint32_t>(1,100));
    }

    // Set up Vulkan engine
    Engine
    vulkan_engine;
    vulkan_engine.init(window);

    // set up the application data
    *appstate = new AppContext{
        vulkan_engine,
        new KeyListener,
        new MouseListener()
    };

    LOGGER.info("Session ID: ${}", vulkan_engine.sid);
    LOGGER.success("Application started successfully!");

    SDL_GetRelativeMouseState(&vulkan_engine.mousePosition.x, &vulkan_engine.mousePosition.y);
    SDL_GetRelativeMouseState(&vulkan_engine.mousePointerPosition.x, &vulkan_engine.mousePointerPosition.y);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e) {
    auto* app = static_cast<AppContext *>(appstate);

    // Close on hitting button
    if (e->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    // Close on ESC
    if(app->engine.quit) {
        return SDL_APP_SUCCESS;
    }

    if (e->type == SDL_EVENT_WINDOW_RESIZED) {
        int width, height;
        SDL_GetWindowSizeInPixels(app->engine.window, &width, &height);
        app->engine.framebufferResized = true;
    }

    app->kL->listen(e);

    if (SDL_GetWindowRelativeMouseMode(app->engine.window)) {
        app->mL->listen(app->engine.camera);
    } else {
        ImGui_ImplSDL3_ProcessEvent(e);
    }

    return SDL_APP_CONTINUE;
}


static std::chrono::duration<double> duration;

SDL_AppResult SDL_AppIterate(void *appstate) {
    const auto start = std::chrono::high_resolution_clock::now();

    auto* app = static_cast<AppContext *>(appstate);
    const auto& sleepTime = app->engine.sleepTimeTotalSeconds;

    app->engine.delta.calculateDelta();
    app->kL->iterateKeys(app->engine);

    app->engine.drawFrame();
    vkDeviceWaitIdle(app->engine.device);

    duration = std::chrono::high_resolution_clock::now() - start;

    // Framerate limiter
    if (duration.count() > sleepTime) return SDL_APP_CONTINUE;

    std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime - duration.count() - sleepTime / 32)); // Rough sleep

    // Precise sleep loop
    while (duration.count() < sleepTime) {
        duration = std::chrono::high_resolution_clock::now() - start;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    // Cleanup
    if (auto* app = static_cast<AppContext *>(appstate)) {
        app->engine.cleanup();
        SDL_DestroyWindow(app->engine.window);
        delete app;
    }

    SDL_Quit();
    LOGGER.success("Application quit successfully!");
}
