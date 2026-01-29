#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

#include "UniformBufferObject.hpp"
#include "UniformCullingBufferObject.hpp"
#include "UniformPostprocessingBufferObject.hpp"
#include "DrawCommandsBufferObject.hpp"
#include "ModelCullingBufferObject.hpp"
#include "VisibleIndicesBufferObject.hpp"
#include "AtomicCounterBufferObject.hpp"
#include "ModelBufferObject.hpp"
#include "ModelDataBufferObject.hpp"
#include "TextureIndexBufferObject.h"
#include "TextureIndexOffsetBufferObject.hpp"

#include "Camera.hpp"
#include "DeltaManager.hpp"
#include "Color.hpp"
#include "ModelBus.hpp"
#include "ModelEntityManager.hpp"
#include "PhysicsBus.h"




struct CullingPushConstants;
// Parameters
inline int HEIGHT = 720;
inline int WIDTH = 1280;

class Engine {
public:
    // Method for initializing and running engine
    #pragma region Main
    void init(SDL_Window* sdl_window);

    void drawImGui(const VkCommandBuffer& commandBuffer);

    void recordCommandBuffer(uint32_t imageIndex, VkPipeline postprocessPipeline);


    void waitForFences(const VkFence& fence) const;

    // Draw
    void drawFrame();


    // Clean trash before closing app
    void cleanup() const;
    #pragma endregion

    #pragma region Buffers
    // Generic
    void updateUniformBuffer();

    void updateUniformPostprocessingBuffer();


    static void normalizePlane(glm::vec4& plane);

    static glm::vec4 extractPlane(glm::mat4 viewProjection, int a, float sign);

    void updateCullingUniformBuffer();


    // Matrices
    void updateModelDataBuffer();

    void updateModelBuffer();

    void updateSingleModel(const std::string &name, uint32_t instanceIndex, glm::mat4 matrix);

    // Culling
    void updateModelCullingBuffer();

    void updateVisibleIndicesBuffer();

    void updateDrawCommands();

    void updateTextureIndexBuffer();
    #pragma endregion

    void recreatePostprocessingPipeline(const std::string& filename);

    #pragma region Public variables
    // SDL
    SDL_Window* window{};

    // Session ID
    uint64_t sid{};


    Color clear_color = Color::hex(0x9c9c9c);

    // Devices
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    bool framebufferResized = false;
    bool quit = false;

    // Generic
    UniformBufferObject ubo{};
    UniformBufferObject umbo{};
    UniformCullingBufferObject ucbo{};
    UniformPostprocessingBufferObject upbo{};
    AtomicCounterBufferObject atomic_counter{};
    DrawCommandsBufferObject dc{};
    DrawCommandsBufferObject dcs{};

    // Matrices
    ModelDataBufferObject mdbo{};
    ModelBufferObject mbo{};

    // Culling
    VisibleIndicesBufferObject vio{};
    ModelCullingBufferObject mcbo{};

    // Controls
    Camera camera{};

    // Texture index
    TextureIndexOffsetBufferObject tioo{};
    TextureIndexBufferObject tio{};

    DeltaManager delta;

    ModelEntityManager mem{};

    double desiredFrameRate;
    double sleepTimeTotalSeconds;

    bool depth = false;
    bool initialized = false;

    glm::vec2 mousePosition;
    glm::vec2 mousePointerPosition;

    VkPhysicalDeviceSubgroupProperties subgroupProperties;

    #pragma endregion PublicVars
private:
    #pragma region Variables
    // Instance
    VkInstance instance{};

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger{};

    // Logger
    Logger LOGGER;

    // Window
    VkSurfaceKHR surface{};

    // Queues
    VkQueue presentQueue{};
    VkQueue graphicsQueue{};

    // Swapchain
    VkSwapchainKHR swapchain{};

    VkFormat swapchainImageFormat{};
    VkExtent2D swapchainExtent{};

    std::vector<VkImage> swapchainImages{};
    std::vector<VkImageView> swapchainImageViews{};

    // Postprocess
    std::vector<VkImage> offscreenImages{};
    std::vector<VkDeviceMemory> offscreenImagesMemory{};
    std::vector<VkImageView> offscreenImageViews{};

    // Graphics
    VkPipeline graphicsPipeline{};
    VkPipelineLayout graphicsPipelineLayout{};
    VkCommandPool graphicsCommandPool{};

    VkDescriptorSetLayout graphicsDescriptorSetLayout{};
    std::vector<VkDescriptorSet> graphicsDescriptorSets{};
    VkDescriptorPool graphicsDescriptorPool{};

    // Matrix
    VkPipeline matrixComputePipeline{};
    VkPipelineLayout matrixComputePipelineLayout{};
    VkCommandPool matrixComputeCommandPool{};
    bool matrixDirty = true;

    // Culling
    VkPipeline cullingComputePipeline{};
    VkPipelineLayout cullingComputePipelineLayout{};
    VkCommandPool cullingComputeCommandPool{};

    // Graphics
    std::string selectedShader = "hdr_fog.spv";
    std::unordered_map<std::string, VkPipeline> postprocessPipelines;
    VkPipelineLayout postprocessPipelineLayout{};
    VkDescriptorSetLayout postprocessDescriptorSetLayout{};

    std::vector<VkDescriptorSet> postprocessDescriptorSets{};
    VkDescriptorPool postprocessDescriptorPool{};
    VkCommandPool postprocessCommandPool{};

    // Depth
    VkImage depthImage{};
    VkDeviceMemory depthImageMemory{};
    VkImageView depthImageView{};

    // Buffers
    // Main
    std::vector<VkCommandBuffer> commandBuffers{};

    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexBufferMemory{};

    VkBuffer indexBuffer{};
    VkDeviceMemory indexBufferMemory{};

    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingBufferMemory{};

    std::vector<VkBuffer> drawCommandsSourceBuffers;
    std::vector<VkDeviceMemory> drawCommandsSourceBuffersMemory;
    std::vector<void*> drawCommandsSourceBuffersMapped{};
    std::vector<uint64_t> drawCommandsSourceConstants;

    std::vector<VkBuffer> drawCommandsBuffers;
    std::vector<VkDeviceMemory> drawCommandsBuffersMemory;
    std::vector<void*> drawCommandsBuffersMapped{};
    std::vector<uint64_t> drawCommandsConstants;

    // Generic
    std::vector<VkBuffer> uniformBuffers{};
    std::vector<VkDeviceMemory> uniformBuffersMemory{};
    std::vector<void*> uniformBuffersMapped{};
    std::vector<uint64_t> uniformConstants{};

    std::vector<VkBuffer> uniformMatrixBuffers{};
    std::vector<VkDeviceMemory> uniformMatrixBuffersMemory{};
    std::vector<void*> uniformMatrixBuffersMapped{};
    std::vector<uint64_t> uniformMatrixConstants{};

    std::vector<VkBuffer> uniformPostprocessingBuffers{};
    std::vector<VkDeviceMemory> uniformPostprocessingBuffersMemory{};
    std::vector<void*> uniformPostprocessingBuffersMapped{};
    std::vector<uint64_t> uniformPostprocessingConstants{};

    std::vector<VkBuffer> uniformCullingBuffers{};
    std::vector<VkDeviceMemory> uniformCullingBuffersMemory{};
    std::vector<void*> uniformCullingBuffersMapped{};
    std::vector<uint64_t> uniformCullingConstants{};

    std::vector<VkBuffer> atomicCounterBuffers{};
    std::vector<VkDeviceMemory> atomicCounterBuffersMemory{};
    std::vector<void*> atomicCounterBuffersMapped{};
    std::vector<uint64_t> atomicCounterConstants{};


    // Matrices
    std::vector<VkBuffer> modelBuffers{};
    std::vector<VkDeviceMemory> modelBuffersMemory{};
    std::vector<void*> modelBuffersMapped{};
    std::vector<uint64_t> modelConstants{};

    std::vector<VkBuffer> modelDataBuffers{};
    std::vector<VkDeviceMemory> modelDataBuffersMemory{};
    std::vector<void*> modelDataBuffersMapped{};
    std::vector<uint64_t> modelDataConstants{};


    // Culling
    std::vector<VkBuffer> visibleIndicesBuffers{};
    std::vector<VkDeviceMemory> visibleIndicesBuffersMemory{};
    std::vector<void*> visibleIndicesBuffersMapped{};
    std::vector<uint64_t> visibleIndicesConstants{};

    VkBuffer modelCullingBuffer{};
    VkDeviceMemory modelCullingBufferMemory{};
    void* modelCullingBufferMapped{};
    uint64_t modelCullingConstant{};

    // Index
    VkBuffer textureIndexBuffer{};
    VkDeviceMemory textureIndexBufferMemory{};
    void* textureIndexBufferMapped{};
    uint64_t textureIndexConstant{};

    VkBuffer textureIndexOffsetBuffer{};
    VkDeviceMemory textureIndexOffsetBufferMemory{};
    void* textureIndexOffsetBufferMapped{};
    uint64_t textureIndexOffsetConstant{};

    // Image
    VkImage textureImage{};
    VkDeviceMemory textureImageMemory{};
    VkImageView textureImageView{};

    VkSampler textureSampler{};

    std::vector<VkSemaphore> imageAvailableSemaphores{};
    std::vector<VkSemaphore> renderFinishedSemaphores{};
    std::vector<VkFence> inFlightFences{};
    std::vector<VkFence> imageFences{};

    // Frames
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;
    uint32_t currentFrame = 0;
    static inline bool initializedMbo = false;

    // Constants

    // modelCullingBuffer, visibleIndicesBuffers, drawCommandsSourceBuffer, uniformCullingBuffers, atomicCounterBuffers
    //std::array<CullingPushConstants, MAX_FRAMES_IN_FLIGHT> cullingAddress;
    #pragma endregion

    #pragma region Initialization
    // Initialization of engine and its counterparts
    void initialize(SDL_Window* window);

    // Creating Vulkan instance
    void createInstance();

    void initializeOffscreenImages(uint32_t width, uint32_t height);

    void initializePostprocessPipelines();
    // Vulkan related stuff
    void initializeVulkan();

    // ImGui Stuff
    void initializeImGui(SDL_Window* sdl_window);

    #pragma endregion

    #pragma region Swapchain
    // Swapchain
    void recreateSwapchain();

    void cleanupSwapchain() const;

    void cleanupOffscreenImages() const;
    #pragma endregion

    #pragma region Helper
    // Setup debug messenger
    void setupDebugMessenger();

    // Get SDL extensions
    static std::vector<const char*> getRequiredExtensions();
#pragma endregion
};
#endif //ENGINE_HPP
