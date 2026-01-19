#ifndef INC_2G43S_MODELBUS_HPP
#define INC_2G43S_MODELBUS_HPP
#include <map>
#include <memory>
#include <vector>

#include "util/Logger.hpp"
#include "../ModelInstance.hpp"
#include "../ParsedModel.hpp"
#include "../../util/Random.hpp"

struct ModelGroup {
    std::shared_ptr<ParsedModel> model{};
    std::vector<ModelInstance> instances{};

    ModelGroup() = default;

    ModelGroup(const std::shared_ptr<ParsedModel> &model, const std::vector<ModelInstance> &instances) : model(model), instances(instances) {}

    explicit ModelGroup(const std::shared_ptr<ParsedModel> &model) : model(model) {}
};

struct ModelBus {
    ModelBus();

    #pragma region Variables
    constexpr static uint32_t MAX_MODELS = 4294967295;
    bool dirtyCommands = true;
    Logger LOGGER;

    std::map<std::string, ModelGroup> groups_map;
    std::vector<VkDrawIndexedIndirectCommand> drawCommands{};
    #pragma endregion


    #pragma region parsedModels
    template<std::convertible_to<std::string>... T>
    void loadModels(const std::string& location, const T&... files);

    void loadModelTextures(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkPhysicalDevice& physicalDevice, VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory);

    template<std::convertible_to<std::string>... T>
    void destroyModels(const T&... files);

    std::shared_ptr<ParsedModel> getModel(const std::string& file);
    #pragma endregion


    #pragma region instanceModels
    void instance(const std::string& file, const auto&... args);
    #pragma endregion


    #pragma region buffers
    VkDeviceSize getIndexBufferSize() const;

    VkDeviceSize getVertexBufferSize() const;

    VkDeviceSize getModelBufferSize() const;

    VkDeviceSize getModelDataBufferSize() const;
    #pragma endregion


    #pragma region data
    // Model loading methods
    std::vector<uint32_t> getAllIndices() const;

    std::vector<uint32_t> getIndices(const std::string& file) const;


    std::vector<Vertex> getAllVertices() const;

    std::vector<Vertex> getVertices(const std::string& file) const;
    #pragma endregion


    #pragma region count
    size_t modelsCount() const;


    size_t getInstanceCount(const std::string& file) const;

    size_t getTotalInstanceCount() const;

    size_t getTotalModelCount() const;



    uint32_t getIndexCount(const std::string& name) const;

    static uint32_t getIndexCount(const std::shared_ptr<ParsedModel>& model);


    int32_t getVertexCount(const std::string& name) const;

    static int32_t getVertexCount(const std::shared_ptr<ParsedModel>& model);
    #pragma endregion


    #pragma region commands
    void addCommands(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t  vertexOffset, uint32_t firstInstance);
    #pragma endregion


    void randomVolume(size_t count, const std::string& file, float min, float max);

    void randomVolume(size_t count, const std::string& file, const glm::vec3& min, const glm::vec3& max);

    void square(size_t count, const std::string& file, double gap);

    void loadModels();
};

#endif //INC_2G43S_MODELBUS_HPP