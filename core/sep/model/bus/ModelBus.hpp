#ifndef INC_2G43S_MODELBUS_HPP
#define INC_2G43S_MODELBUS_HPP
#include <memory>
#include <vector>

#include "../ModelInstance.hpp"
#include "../ParsedModel.hpp"
#include <omp.h>


#include "../../util/Random.hpp"

struct ModelBus {
    constexpr static uint32_t MAX_MODELS = 4294967295;
    bool dirtyCommands = true;
    Logger* LOGGER;

    std::vector<std::shared_ptr<ParsedModel>> mdls{};
    std::vector<ModelInstance> mdls_i{};

    std::vector<VkDrawIndexedIndirectCommand> drawCommands{};


    ModelBus();


    #pragma region parsedModels
    void loadModel(const std::string& location, const std::string& file);

    void loadModels(const std::string& location, const std::vector<std::string>& files);


    void unloadModel(const std::string& file);

    void unloadModels(const std::vector<std::string>& files);

    std::shared_ptr<ParsedModel> getModel(const std::string& name) const;
    #pragma endregion


    #pragma region instanceModels
    void addModelInstance(const std::string& model, glm::vec4 p = glm::vec4(0.0f), glm::vec4 r = glm::vec4(0.0f), glm::vec4 s = glm::vec4(1.0f));

    void addModelInstance(const std::shared_ptr<ParsedModel>& mdl, glm::vec4 p = glm::vec4(0.0f), glm::vec4 r = glm::vec4(0.0f), glm::vec4 s = glm::vec4(1.0f));

    static ModelInstance createModelInstance(const std::shared_ptr<ParsedModel>& mdl, glm::vec4 p = glm::vec4(0.0f), glm::vec4 r = glm::vec4(0.0f), glm::vec4 s = glm::vec4(1.0f));

    void destroyModelInstance(const std::string& name);


    #pragma endregion


    #pragma region buffers
    VkDeviceSize getIndexBufferSize() const;

    VkDeviceSize getVertexBufferSize() const;


    std::vector<uint32_t> getIndices() const;

    std::vector<Vertex> getVertices() const;

    #pragma endregion


    #pragma region helper
    size_t modelsCount() const;

    size_t getInstanceCount(const std::shared_ptr<ParsedModel>& model) const;

    uint32_t getIndexCount(const std::string& name) const;

    static uint32_t getIndexCount(const std::shared_ptr<ParsedModel>& model);

    uint32_t getVertexCount(const std::string& name) const;

    static int32_t getVertexCount(const std::shared_ptr<ParsedModel>& model);

    #pragma endregion


    #pragma region commands

    void addCommands(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t  vertexOffset, uint32_t firstInstance);
    #pragma endregion


    void test();
};

#endif //INC_2G43S_MODELBUS_HPP