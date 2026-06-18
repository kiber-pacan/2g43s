#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include <cmath>
#include <thread>

#define GLM_FORCE_RADIANS
#include <imgui_impl_sdl3.h>
#include <glm/glm.hpp>
#include <omp.h>
#include <basisu_transcoder.h>

#include "KeyListener.hpp"
#include "MouseListener.hpp"
#include "Random.hpp"
#include "Engine.hpp"
#include "Logger.hpp"


class PhysicsBus;
Logger LOGGER = Logger("main.cpp");

struct AppContext {
    Engine* engine;
    KeyListener* kL;
    MouseListener* mL;

    SDL_Thread* tickThread = nullptr;
    double desiredFrameRate = 200;
    uint64_t sleepTimeTotalNS = 1 / desiredFrameRate * 1'000'000'000.0;
};

int TickThread(void* ptr) {
    constexpr uint TICK_RATE = 256;

    auto* app = static_cast<AppContext*>(ptr);
    constexpr double dt = 1.0 / TICK_RATE;

    while (!app->engine->quit) {
        const uint64_t start = SDL_GetTicksNS();

        //if (app->engine->initialized) app->engine->modelEntityManager.physicsBus.iterateJPH(*app->engine);

        const uint64_t elapsed = SDL_GetTicksNS() - start;
        double sleepTime = dt - static_cast<double>(elapsed) / 1'000'000'000.0;
        if (sleepTime > 0) SDL_DelayNS(static_cast<Uint64>(sleepTime * 1'000'000'000.0));
    }
    return 0;
}

#pragma region SDL3
SDL_AppResult SDL_Fail() {
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    #if defined(__linux__)
        setenv("OMP_PROC_BIND", "true", 1);
        setenv("OMP_PLACES", "cores", 1);
    #endif

    basist::basisu_transcoder_init();

    // Init the library, here we make a window so we only need the Video capabilities.
    if (!SDL_Init(SDL_INIT_VIDEO)){
        return SDL_Fail();
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Window", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
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
    Engine* vulkan_engine = new Engine();

    // set up the application data
    *appstate = new AppContext{
        vulkan_engine,
        new KeyListener{},
        new MouseListener{},
    };

    auto* app = static_cast<AppContext*>(*appstate);

    vulkan_engine->desiredFrameRate = &app->desiredFrameRate;
    vulkan_engine->sleepTimeTotalNS = &app->sleepTimeTotalNS;
    vulkan_engine->modelEntityManager.physicsBus.initializeJPH();
    vulkan_engine->initialize(window);

    app->tickThread = SDL_CreateThread(TickThread, "TickThread", app);

    if (!app->tickThread) {
        LOGGER.error("Failed to INITIALIZE tick thread!");
        return SDL_APP_FAILURE;
    }

    LOGGER.info("Session ID: ${}", vulkan_engine->sid);
    LOGGER.success("Application STARTED successfully!");

    SDL_GetRelativeMouseState(&vulkan_engine->mousePosition.x, &vulkan_engine->mousePosition.y);
    SDL_GetRelativeMouseState(&vulkan_engine->mousePointerPosition.x, &vulkan_engine->mousePointerPosition.y);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e) {
    auto* app = static_cast<AppContext *>(appstate);

    // Close on hitting button
    if (e->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    // Close on ESC
    if(app->engine->quit) {
        return SDL_APP_SUCCESS;
    }

    if (e->type == SDL_EVENT_WINDOW_RESIZED) {
        int width, height;
        SDL_GetWindowSizeInPixels(app->engine->window, &width, &height);
        app->engine->framebufferResized = true;
    }

    app->kL->listen(e);

    if (SDL_GetWindowRelativeMouseMode(app->engine->window)) {
        app->mL->listen(app->engine->camera);
    } else {
        ImGui_ImplSDL3_ProcessEvent(e);
    }

    if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (e->button.button == SDL_BUTTON_LEFT) {
            glm::vec3 impulse = app->engine->camera.look;
            impulse*=16;
            //app->engine->modelEntityManager.physicsInstance("box.glb", glm::vec4(app->engine->camera.pos, 1), 256);
            app->engine->bufferManager.addModel(
                "box.glb",
                glm::vec4(
                    app->engine->camera.pos.x,
                    app->engine->camera.pos.y,
                    app->engine->camera.pos.z,
                    0
                    )
                );
        }
    }

    return SDL_APP_CONTINUE;
}


static uint64_t renderDurationNs;

SDL_AppResult SDL_AppIterate(void *appstate) {
    const uint64_t start = SDL_GetTicksNS();

    const auto* app = static_cast<AppContext *>(appstate);
    const uint64_t& sleepTime = app->sleepTimeTotalNS;

    app->engine->delta.calculateDelta();
    app->kL->iterateKeys(*app->engine);

    app->engine->graphicsManager.drawFrame();

    renderDurationNs = SDL_GetTicksNS() - start;

    #pragma region fpsLimit
    if (renderDurationNs > sleepTime) return SDL_APP_CONTINUE;

    SDL_DelayNS(static_cast<uint64_t>(static_cast<long double>(sleepTime - renderDurationNs) / 1.25L)); // Rough sleep

    // Precise sleep loop
    while (renderDurationNs < sleepTime) {
        renderDurationNs = SDL_GetTicksNS() - start;
    }
    #pragma endregion

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    // Cleanup
    if (const auto* app = static_cast<AppContext *>(appstate)) {
        app->engine->modelEntityManager.physicsBus.shutdownJPH();
        app->engine->cleanup();
        SDL_DestroyWindow(app->engine->window);
        delete app;
    }

    SDL_Quit();
    LOGGER.success("Application COMPLETED successfully!");
}
#pragma endregion
