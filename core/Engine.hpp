#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>


#include "Camera.hpp"
#include "ModelBus.hpp"

#include "GraphicsManager.hpp"
#include "SwapchainManager.hpp"
#include "ModelEntityManager.hpp"
#include "BufferManager.hpp"
#include "DeltaManager.hpp"
#include "glmMath.h"


struct CullingPushConstants;
// Parameters
inline int height = 720;
inline int width = 1280;

class Engine {
public:

    // Method for initializing and running engine
    #pragma region Main
    void initialize(SDL_Window* sdl_window);

    // Clean trash before closing app
    void cleanup() const;
    #pragma endregion

    #pragma region Public variables
    // SDL
    SDL_Window* window{};
    VkSurfaceKHR surface{};

    // Session ID
    uint64_t sid{};

    // Devices
    VkDevice device{};
    VkPhysicalDevice physicalDevice{};

    // Managers
    ModelEntityManager modelEntityManager{};
    GraphicsManager graphicsManager{};
    SwapchainManager swapchainManager{};
    BufferManager bufferManager{};

    // Other stuff

    bool depth = false;
    bool initialized = false;
    bool framebufferResized = false;
    bool quit = false;

    glm::vec2 mousePosition{};
    glm::vec2 mousePointerPosition{};

    VkPhysicalDeviceSubgroupProperties subgroupProperties{};

    Camera camera{};

    // Framerate type shi
    double* desiredFrameRate;
    uint64_t* sleepTimeTotalNS;
    DeltaManager delta{};

    #pragma endregion PublicVars
private:
    #pragma region Variables
    // Instance
    VkInstance instance{};

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger{};

    // Logger
    Logger logger;

    // Queues
    VkQueue presentQueue{};
    VkQueue graphicsQueue{};

    VkSampler textureSampler{};


    // Frames
    static constexpr size_t framesInFlight = 3;
    #pragma endregion

    #pragma region Initialization
    void initializeCore();

    #pragma region Vulkan
    // Creating Vulkan instance
    void createVulkanInstance();

    void initializeDevices();

    void initializeVulkan();
    #pragma endregion
    #pragma endregion

    #pragma region Helper
    // Setup debug messenger
    void setupDebugMessenger();

    // Get SDL extensions
    static std::vector<const char*> getRequiredExtensions();
#pragma endregion
};
#endif //ENGINE_HPP
