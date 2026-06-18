//
// Created by down1 on 25.02.2026.
//

#ifndef INC_2G43S_GRAPHICS_H
#define INC_2G43S_GRAPHICS_H


#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Color.hpp"


struct Camera;
struct DeltaManager;
struct ModelEntityManager;
struct SwapchainManager;
struct BufferManager;

struct GraphicsManager {
    explicit GraphicsManager() = default;

    GraphicsManager(
        VkDevice device, VkPhysicalDevice physicalDevice,
        BufferManager* bufferManager, SwapchainManager* swapchainManager, ModelEntityManager* modelEntityManager, DeltaManager* delta,
        VkQueue presentQueue, VkQueue graphicsQueue,
        VkInstance instance,
        bool* framebufferResized, size_t MAX_FRAMES_IN_FLIGHT,
        Camera* camera,
        double* desiredFrameRate, uint64_t* sleepTimeTotalNS
        ) :
    MAX_FRAMES_IN_FLIGHT(MAX_FRAMES_IN_FLIGHT),
    device(device), physicalDevice(physicalDevice),
    bufferManager(bufferManager), swapchainManager(swapchainManager), modelEntityManager(modelEntityManager), delta(delta),
    presentQueue(presentQueue), graphicsQueue(graphicsQueue),
    framebufferResized(framebufferResized),
    instance(instance), camera(camera),
    desiredFrameRate(desiredFrameRate), sleepTimeTotalNS(sleepTimeTotalNS)
    {}


    bool matrixDirty = true;
private:
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

    std::vector<VkSemaphore> imageAvailableSemaphores{};
    std::vector<VkSemaphore> renderFinishedSemaphores{};
    std::vector<VkFence> inFlightFences{};
    std::vector<VkFence> imageFences{};

    Color clear_color = Color::hex(0x9c9c9c);

    uint32_t currentFrame = 0;
    size_t MAX_FRAMES_IN_FLIGHT{};

    // Image
    VkImage missingnoTextureImage{};
    VkDeviceMemory missingnoTextureImageMemory{};
    VkImageView missingnoTextureImageView{};

    #pragma region Dependencies
    VkDevice device;
    VkPhysicalDevice physicalDevice;

    BufferManager* bufferManager;
    SwapchainManager* swapchainManager;
    ModelEntityManager* modelEntityManager;
    DeltaManager* delta;

    VkQueue presentQueue{};
    VkQueue graphicsQueue{};

    bool* framebufferResized;
    VkInstance instance;

    Camera* camera{};

    double* desiredFrameRate;
    uint64_t* sleepTimeTotalNS;
    #pragma endregion


    void drawImGui(const VkCommandBuffer& commandBuffer);

    void recordCommandBuffer(VkPipeline postprocessPipeline, uint32_t imageIndex);

    void waitForFence(VkFence fence) const;

    // Create pipeline for each postprocessing shader
    private: void initializePostprocessPipelines();

    void initializePipelines();

    void initializeDescriptors();

    void initializeTextures();

    void initializeImGui();
public:
    void initialize();

    void recreatePostprocessingPipeline(const std::string& filename);

    void drawFrame();

    void cleanupTextures() const;

    void cleanup() const;
};


#endif //INC_2G43S_GRAPHICS_H