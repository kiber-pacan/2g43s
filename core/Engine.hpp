#ifndef ENGINE_HPP
#define ENGINE_HPP
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <SDL3/SDL_video.h>

#include <vulkan/vulkan_core.h>
#include "shaders/constants/MatrixPushConstants.hpp"
#include "shaders/constants/PostprocessPushConstants.hpp"
#include "shaders/constants/VertexPushConstants.hpp"
#include "shaders/constants/CullingPushConstants.hpp"
#include "ModelCullingBufferObject.hpp"
#include "VisibleIndicesBufferObject.hpp"
#include "AtomicCounterObject.hpp"
#include "DrawCommandsBufferObject.hpp"
#include "UniformBufferObject.hpp"
#include "UniformPostprocessingBufferObject.hpp"
#include "UniformCullingBufferObject.hpp"
#include "ModelDataBufferObject.hpp"
#include "ModelBufferObject.hpp"
#include "Camera.hpp"
#include "DeltaManager.hpp"
#include "ModelBus.hpp"
#include "Color.hpp"
#include "TextureIndexObject.h"
#include "TextureIndexOffsetObject.hpp"
#include "command/Barrier.h"
#include "imgui_internal.h"
#include <ranges>
#include "sep/graphics/command/Command.hpp"
#include "Descriptor.hpp"
#include "Helper.hpp"
#include "Images.hpp"
#include "PipelineCreation.hpp"
#include "Tools.hpp"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "device/LogicalDevice.hpp"
#include "device/PhysicalDevice.hpp"
#include "graphics/Buffers.hpp"
#include "window/Surface.hpp"
#include "sep/graphics/shaders/Shaders.hpp"


struct CullingPushConstants;
// Parameters
inline int HEIGHT = 720;
inline int WIDTH = 1280;

class Engine {
public:
    // Method for initializing and running engine
    #pragma region Main
    void init(SDL_Window* window);

    void drawImGui(const VkCommandBuffer& commandBuffer);

    void recordCommandBuffer(uint32_t imageIndex, VkPipeline postprocessPipeline);

    // Draw
    void drawFrame();


    // Clean trash before closing app
    void cleanup() const;
    #pragma endregion

    #pragma region Buffers
    // Generic
    static void updateUniformBuffer(uint32_t currentFrame, const VkExtent2D& swapchainExtent, const std::vector<void*>& uniformBuffersMapped, const std::vector<void*>& uniformMatrixBuffersMapped, UniformBufferObject& ubo, UniformBufferObject& umbo, const Camera& camera, const ModelBus& mdlBus);

    static void updateUniformPostprocessingBuffer(uint32_t currentFrame, const std::vector<void*>& uniformBuffersMapped, UniformPostprocessingBufferObject& upbo, DeltaManager& delta, const VkExtent2D& swapchainExtent);


    static void normalizePlane(glm::vec4& plane);

    static glm::vec4 extractPlane(glm::mat4 viewProjection, int a, float sign);

    static void updateCullingUniformBuffer(uint32_t currentFrame, const VkExtent2D& swapchainExtent, const std::vector<void*>& uniformCullingBuffersMapped, UniformCullingBufferObject& ucbo, const Camera& camera, const ModelBus& mdlBus);


    // Matrices
    static void updateModelDataBuffer(uint32_t currentFrame, const std::vector<void*>& modelDataBuffersMapped, const ModelBus& mdlBus);

    static void updateModelBuffer(const std::vector<void*>& modelBuffersMapped, ModelBufferObject& mbo, ModelBus& mdlBus);


    // Culling
    static void updateModelCullingBuffer(void*& modelCullingBufferMapped, ModelCullingBufferObject& mcbo, const ModelBus& mdlBus);

    static void updateVisibleIndicesBuffer(const std::vector<void*>& visibleIndicesBuffersMapped, VisibleIndicesBufferObject& vio);

    static void updateDrawCommands(void* drawCommandsBufferMapped, DrawCommandsBufferObject& dc, ModelBus& mdlBus);

    static void updateTextureIndexBuffer(void* TextureIndexBufferMapped, void* TextureIndexOffsetBufferMapped, TextureIndexObject& tio, TextureIndexOffsetObject& tioo, ModelBus& mdlBus);
    #pragma endregion

    void recreatePostprocessingPipeline(const std::string& filename);

    #pragma region Public variables
    // SDL
    SDL_Window* window{};

    // Session ID
    uint64_t sid{};


    Color clear_color = Color::hex(0x000000);

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
    AtomicCounterObject atomic_counter{};
    DrawCommandsBufferObject dc{};

    // Matrices
    ModelDataBufferObject mdbo{};
    ModelBufferObject mbo{};

    // Culling
    VisibleIndicesBufferObject vio{};
    ModelCullingBufferObject mcbo{};

    // Controls
    Camera camera{};

    // Texture index
    TextureIndexOffsetObject tioo{};
    TextureIndexObject tio{};

    DeltaManager delta;

    ModelBus mdlBus{};

    double desiredFrameRate;
    double sleepTimeTotalSeconds;

    bool depth = false;

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

    VkBuffer drawCommandsBuffer{};
    VkDeviceMemory drawCommandsBufferMemory{};
    void* drawCommandsBufferMapped{};
    uint64_t drawCommandsConstant{};

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
    std::vector<VkDeviceMemory> visibleIndicesMemory{};
    std::vector<void*> visibleIndicesMapped{};
    std::vector<uint64_t> visibleIndicesConstants{};

    VkBuffer modelCullingBuffer{};
    VkDeviceMemory modelCullingBufferMemory{};
    void* modelCullingBufferMapped{};
    uint64_t modelCullingConstant{};

    // Index
    VkBuffer textureIndexBuffer{};
    VkDeviceMemory textureIndexMemory{};
    void* textureIndexMapped{};
    uint64_t textureIndexConstant{};

    VkBuffer textureIndexOffsetBuffer{};
    VkDeviceMemory textureIndexOffsetMemory{};
    void* textureIndexOffsetMapped{};
    uint64_t textureIndexOffsetConstant{};

    // Image
    VkImage textureImage{};
    VkDeviceMemory textureImageMemory{};
    VkImageView textureImageView{};

    VkSampler textureSampler{};

    // Fences
    std::vector<VkSemaphore> imageAvailableSemaphores{};
    std::vector<VkSemaphore> renderFinishedSemaphores{};
    std::vector<VkFence> inFlightFences{};

    // Frames
    static constexpr int MAX_FRAMES_IN_FLIGHT = 4;
    uint32_t currentFrame = 0;

    // Constants

    // modelCullingBuffer, visibleIndicesBuffers, drawCommandsBuffer, uniformCullingBuffers, atomicCounterBuffers
    //std::array<CullingPushConstants, MAX_FRAMES_IN_FLIGHT> cullingAddress;
    #pragma endregion

    #pragma region Initialization
    // Initialization of engine and its counterparts
    void initialize(SDL_Window* window);

    // Creating Vulkan instance
    void createInstance();

    void initializeOffscreenImages(int width, int height);

    void initializePostprocessPipelines();
    // Vulkan related stuff
    void initializeVulkan();

    // ImGui Stuff
    void initializeImGui(SDL_Window* window);

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
