#ifndef ENGINE_HPP
#define ENGINE_HPP
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <SDL3/SDL_video.h>

#include <vulkan/vulkan_core.h>

#include "buffers/culling/ModelCullingBufferObject.hpp"
#include "buffers/culling/VisibleIndicesBufferObject.hpp"
#include "buffers/generic/AtomicCounterObject.hpp"
#include "buffers/generic/DrawCommandsBufferObject.hpp"
#include "buffers/generic/UniformBufferObject.hpp"
#include "buffers/generic/UniformCullingBufferObject.hpp"
#include "buffers/matrices/ModelDataBufferObject.hpp"
#include "buffers/matrices/ModelBufferObject.hpp"
#include "camera/Camera.hpp"
#include "graphics/Delta.hpp"
#include "model/bus/ModelBus.hpp"
#include "util/Color.hpp"


// Parameters
inline int HEIGHT = 720;
inline int WIDTH = 1280;

class Engine {
public:
    // Method for initializing and running engine
    #pragma region Main
    void init(SDL_Window* window);

    void drawImGui(const VkCommandBuffer& commandBuffer) const;

    // Draw
    void drawFrame();


    // Clean trash before closing app
    void cleanup() const;
    #pragma endregion

    #pragma region Buffers
    // Generic
    static void updateUniformBuffer(const uint32_t currentFrame, const VkExtent2D& swapchainExtent, const std::vector<void*>& uniformBuffersMapped, UniformBufferObject& ubo, const Camera& camera);

    static void normalizePlane(glm::vec4& plane);

    static glm::vec4 extractPlane(glm::mat4 viewProjection, int a, float sign);

    static void updateCullingUniformBuffer(const uint32_t currentFrame, const VkExtent2D& swapchainExtent, const std::vector<void*>& uniformCullingBuffersMapped, UniformCullingBufferObject& ucbo, const Camera& camera, const ModelBus& mdlBus);


    // Matrices
    static void updateModelDataBuffer(const uint32_t currentFrame, const std::vector<void*>& modelDataBuffersMapped, const ModelBus& mdlBus);

    static void updateModelBuffer(const std::vector<void*>& modelBuffersMapped, ModelBufferObject& mbo);


    // Culling
    static void updateModelCullingBuffer(void*& modelCullingBufferMapped, ModelCullingBufferObject& mcbo, const ModelBus& mdlBus);

    static void updateVisibleIndicesBuffer(const std::vector<void*>& visibleIndicesBuffersMapped, VisibleIndicesBufferObject& vio);

    static void updateDrawCommands(void* drawCommandsBufferMapped, DrawCommandsBufferObject& dc, ModelBus& mdlBus);
    #pragma endregion

    #pragma region PublicVars
    // SDL
    SDL_Window* window{};

    // Session ID
    uint64_t sid{};


    Color clear_color = Color::hex(0x80c4b5);

    // Devices
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    bool framebufferResized = false;

    // Generic
    UniformBufferObject ubo{};
    UniformCullingBufferObject ucbo{};
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

    Delta *deltaT;

    ModelBus mdlBus{};
    #pragma endregion PublicVars

private:
    #pragma region Variables
    // Instance
    VkInstance instance{};

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger{};

    // Logger
    Logger* LOGGER{};

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

    // Graphics
    VkPipeline graphicsPipeline{};
    VkPipelineLayout graphicsPipelineLayout{};
    VkDescriptorSetLayout graphicsDescriptorSetLayout{};

    std::vector<VkDescriptorSet> graphicsDescriptorSets{};
    VkDescriptorPool graphicsDescriptorPool{};
    VkCommandPool graphicsCommandPool{};

    // Matrix
    VkPipeline matrixComputePipeline{};
    VkPipelineLayout matrixComputePipelineLayout{};
    VkDescriptorSetLayout matrixComputeDescriptorSetLayout{};
    bool matrixDirty = true;

    std::vector<VkDescriptorSet> matrixComputeDescriptorSets{};
    VkDescriptorPool matrixComputeDescriptorPool{};
    VkCommandPool matrixComputeCommandPool{};

    // Culling
    VkPipeline cullingComputePipeline{};
    VkPipelineLayout cullingComputePipelineLayout{};
    VkDescriptorSetLayout cullingComputeDescriptorSetLayout{};

    std::vector<VkDescriptorSet> cullingComputeDescriptorSets{};
    VkDescriptorPool cullingComputeDescriptorPool{};
    VkCommandPool cullingComputeCommandPool{};

    // Graphics
    VkPipeline postprocessPipeline{};
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

    // Generic
    std::vector<VkBuffer> uniformBuffers{};
    std::vector<VkDeviceMemory> uniformBuffersMemory{};
    std::vector<void*> uniformBuffersMapped{};

    std::vector<VkBuffer> uniformCullingBuffers{};
    std::vector<VkDeviceMemory> uniformCullingBuffersMemory{};
    std::vector<void*> uniformCullingBuffersMapped{};

    std::vector<VkBuffer> atomicCounterBuffers{};
    std::vector<VkDeviceMemory> atomicCounterBuffersMemory{};
    std::vector<void*> atomicCounterBuffersMapped{};


    // Matrices
    std::vector<VkBuffer> modelBuffers{};
    std::vector<VkDeviceMemory> modelBuffersMemory{};
    std::vector<void*> modelBuffersMapped{};

    std::vector<VkBuffer> modelDataBuffers{};
    std::vector<VkDeviceMemory> modelDataBuffersMemory{};
    std::vector<void*> modelDataBuffersMapped{};


    // Culling
    std::vector<VkBuffer> visibleIndicesBuffers{};
    std::vector<VkDeviceMemory> visibleIndicesMemory{};
    std::vector<void*> visibleIndicesMapped{};

    VkBuffer modelCullingBuffer{};
    VkDeviceMemory modelCullingBufferMemory{};
    void* modelCullingBufferMapped{};

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

    // Shader stuff

    #pragma endregion

    #pragma region Initialization
    // Initialization of engine and its counterparts
    void initialize(SDL_Window* window);

    // Creating Vulkan instance
    void createInstance();

    // Vulkan related stuff
    void initializeVulkan();

    // ImGui Stuff
    void initializeImGui(SDL_Window* window);

    #pragma endregion

    #pragma region Swapchain
    // Swapchain
    void recreateSwapchain();

    void cleanupSwapchain() const;
    #pragma endregion

    #pragma region Helper
    // Setup debug messenger
    void setupDebugMessenger();

    // Get SDL extensions
    static std::vector<const char*> getRequiredExtensions();
    #pragma endregion
};
#endif //ENGINE_HPP
