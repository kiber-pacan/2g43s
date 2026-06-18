//
// Created by down1 on 24.02.2026.
//

#ifndef INC_2G43S_BUFFERBUS_H
#define INC_2G43S_BUFFERBUS_H



#include <string>
#include "Types.hpp"

struct SwapchainManager;
struct DeltaManager;
struct ModelEntityManager;
struct Camera;
struct UniformBuffer;

struct BufferManager {
    explicit BufferManager() = default;

    BufferManager(
        VkDevice device, VkPhysicalDevice physicalDevice,
        ModelEntityManager* modelEntityManager, SwapchainManager* swapchainManager,
        Camera* camera,
        size_t MAX_FRAMES_IN_FLIGHT
        ) :
    device(device), physicalDevice(physicalDevice),
    modelEntityManager(modelEntityManager), swapchainManager(swapchainManager),
    camera(camera), MAX_FRAMES_IN_FLIGHT(MAX_FRAMES_IN_FLIGHT) {}

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    ModelEntityManager* modelEntityManager;
    SwapchainManager* swapchainManager;
    Camera* camera;
    size_t MAX_FRAMES_IN_FLIGHT;

    #pragma region Variables
    // Generic
    UniformBuffer uniformBufferObject{};
    UniformBuffer uniformModelBufferObject{};

    UniformCullingBuffer uniformCullingBufferObject{};
    UniformPostprocessingBuffer uniformPostprocessingBufferObject{};
    AtomicCounterBuffer atomicCounterObject{};
    DrawCommandsBuffer drawCommandsObject{};
    DrawCommandsBuffer drawCommandsSourceObject{};

    // Matrices
    MatrixDataBufferObject matDataBufferObject{};
    MatrixBufferObject matBufferObject{};

    // Culling
    VisibleIndicesBuffer visibleIndicesObject{};
    matrixCullingBuffer matCullingBufferObject{};

    // Texture index
    TextureIndexOffsetBuffer TextureIndexOffsetBufferObject{};
    TextureIndexBuffer textureIndexBufferObject{};

    // BuffersRegistry
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

    static inline bool modelBufferInitialized = false;
    #pragma endregion


    #pragma region Update
    // Generic
    void updateUniformBuffer(uint32_t currentFrame);

    void updateUniformPostprocessingBuffer(uint32_t currentFrame, const DeltaManager& delta);


    static void normalizePlane(glm::vec4& plane);

    static glm::vec4 extractPlane(glm::mat4 viewProjection, int a, float sign);

    void updateCullingUniformBuffer(uint32_t currentFrame);


    // Matrices
    void updateModelDataBuffer(uint32_t currentFrame);

    void initializeModelBuffer();

    void updateModelBuffer();

    void updateSingleModel(const std::string &name, uint32_t instanceIndex, glm::mat4 matrix);

    void addModel(const std::string &name, glm::vec4 pos);

    // Culling
    void updateModelCullingBuffer();

    void updateVisibleIndicesBuffer();

    void updateDrawCommands();

    void updateTextureIndexBuffer();
    #pragma endregion

    void createBuffers(VkQueue graphicsQueue, VkCommandPool graphicsCommandPool);

    void cleanup() const;
};


#endif //INC_2G43S_BUFFERBUS_H