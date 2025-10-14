#ifndef INC_2G43S_MODELBUS_H
#define INC_2G43S_MODELBUS_H
#include <memory>
#include <vector>

#include "ModelInstance.hpp"
#include "ParsedModel.hpp"
#include <omp.h>

#include "../util/Random.hpp"

struct ModelBus {
    std::vector<std::shared_ptr<ParsedModel>> mdls{};
    std::vector<ModelInstance> mdls_i{};

    std::vector<VkDrawIndexedIndirectCommand> drawCommands{};
    bool dirtyCommands = true;

    constexpr static
    uint32_t MAX_MODELS = 4294967295;

    Logger* LOGGER;

    ModelBus() {
        LOGGER = Logger::of("ModelBus");
    }

    //TODO: CONSIDER SWITCHING TO BETTER ALTERNATIVE!!!
    /*
    mdls.erase(
    std::remove_if(mdls.begin(), mdls.end(),
                   [&](const auto& m){ return m->name == file; }),
    mdls.end()
    );
    */

    #pragma region parsedModels
    void loadModel(const std::string& location, const std::string& file) {
        std::filesystem::path path = location + file;
        mdls.emplace_back(std::make_shared<ParsedModel>(path));
    }

    void loadModels(const std::string& location, const std::vector<std::string>& files) {
        for (const auto& name : files) {
            std::filesystem::path path = location + name;
            mdls.emplace_back(std::make_shared<ParsedModel>(path));
        }
    }


    void unloadModel(const std::string& file) {
        for (int i = 0; i < mdls.size(); ++i) {
            if (mdls[i]->name == file) {
                mdls.erase(mdls.begin() + i);
            }
        }
    }

    void unloadModels(const std::vector<std::string>& files) {
        for (int i = 0; i < mdls.size(); ++i) {
            for (const std::string& file : files) {
                if (mdls[i]->name == file) {
                    mdls.erase(mdls.begin() + i);
                }
            }
        }
    }

    std::shared_ptr<ParsedModel> getModel(const std::string& name) const {
        std::shared_ptr<ParsedModel> model;

        for (const auto& mdl: mdls) {
            if (mdl->name == name) {
                model = mdl;
            }
        }

        return model;
    }
    #pragma endregion


    #pragma region instanceModels
    void addModelInstance(const std::string& model, const glm::vec4 p = glm::vec4(0.0f), const glm::vec4 r = glm::vec4(0.0f), const glm::vec4 s = glm::vec4(1.0f)) {
        for (const std::shared_ptr<ParsedModel> & mdl: mdls) {
            if (mdl->name == model) {
                mdls_i.emplace_back(mdl, p, r, s);
            }
        }
    }

    void addModelInstance(const std::shared_ptr<ParsedModel>& mdl, const glm::vec4 p = glm::vec4(0.0f), const glm::vec4 r = glm::vec4(0.0f), const glm::vec4 s = glm::vec4(1.0f)) {
        mdls_i.emplace_back(mdl, p, r, s);
    }

    static ModelInstance createModelInstance(const std::shared_ptr<ParsedModel>& mdl, const glm::vec4 p = glm::vec4(0.0f), const glm::vec4 r = glm::vec4(0.0f), const glm::vec4 s = glm::vec4(1.0f)) {
        return ModelInstance(mdl, p, r, s);
    }

    void destroyModelInstance(const std::string& name) {
        /*for (int i = 0; i < mdls_i.size(); ++i) {
            if (mdls_i[i].name == name) {
                mdls_i.erase(mdls_i.begin() + i);
            }
        }*/
    }


    #pragma endregion


    #pragma region buffers
    VkDeviceSize getIndexBufferSize() const {
        VkDeviceSize bufferSize = 0;
        for (const auto& mdl: mdls) {
            bufferSize += sizeof(mdl->indices[0]) * mdl->indices.size();
        }

        return bufferSize;
    }

    VkDeviceSize getVertexBufferSize() const {
        VkDeviceSize bufferSize = 0;
        for (const auto& mdl: mdls) {
            for (const auto& mesh: mdl->meshes) {
                bufferSize += sizeof(mesh[0]) * mesh.size();
            }
        }

        return bufferSize;
    }


    std::vector<uint32_t> getIndices() const {
        std::vector<uint32_t> indices{};
        for (const auto& mdl: mdls) {
            indices.append_range(mdl->indices);
        }

        for (uint32_t index : indices) {
            //std::cout << index << std::endl;
        }

        return indices;
    }

    std::vector<Vertex> getVertices() const {
        std::vector<Vertex> vertices{};
        for (const auto& mdl: mdls) {
            for (const auto& mesh: mdl->meshes) {
                //vertices.append_range(mesh);
            }
        }

        for (Vertex vertex : vertices) {
            std::cout << vertex.pos.x << " " << vertex.pos.y << " " << vertex.pos.z << std::endl;
        }

        return vertices;
    }

    #pragma endregion


    #pragma region helper
    size_t modelsCount() const {
        return mdls.size();
    }

    size_t getInstanceCount(const std::shared_ptr<ParsedModel>& model) const {
        size_t instanceCount = 0;
        for (const auto& mdl: mdls_i) {
            instanceCount += mdl.mdl == model ? 1 : 0;
        }

        return instanceCount;
    }

    uint32_t getIndexCount(const std::string& name) const {
        if (const std::shared_ptr<ParsedModel> model = getModel(name); model != nullptr) {
            return model->indices.size();
        }

        return 0;
    }

    static uint32_t getIndexCount(const std::shared_ptr<ParsedModel>& model) {
        return model->indices.size();
    }

    uint32_t getVertexCount(const std::string& name) const {
        uint32_t vertexCount = 0;
        if (const std::shared_ptr<ParsedModel> model = getModel(name); model != nullptr) {
            for (const auto& mesh: model->meshes) {
                vertexCount += mesh.size();
            }
        }

        return vertexCount;
    }

    static int32_t getVertexCount(const std::shared_ptr<ParsedModel>& model) {
        int32_t vertexCount = 0;
        for (const auto& mesh: model->meshes) {
            vertexCount += mesh.size();
        }

        return vertexCount;
    }

    #pragma endregion


    #pragma region commands

    void addCommands(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const int32_t  vertexOffset, const uint32_t firstInstance) {
        VkDrawIndexedIndirectCommand drawCommand{};
        drawCommand.indexCount = indexCount;
        drawCommand.instanceCount = instanceCount;
        drawCommand.firstIndex = firstIndex;
        drawCommand.vertexOffset = vertexOffset;
        drawCommand.firstInstance = firstInstance;

        drawCommands.emplace_back(drawCommand);
    }
    #pragma endregion



    void test() {
        loadModel("/mnt/sda1/CLionProjects/2g43s/core/models/", "cube_1_material.glb");
        //loadModel("/mnt/sda1/CLionProjects/2g43s/core/models/", "landscape.glb");

        auto start = std::chrono::high_resolution_clock::now();
        static size_t count = 10000000;
        mdls_i.resize(count);

        #pragma omp single
        std::cout << "Threads: " << omp_get_num_threads() << std::endl;

        #pragma omp parallel for default(none) shared(mdls, count)
        for (int i = 0; i < count; ++i) {
            const glm::vec4 temp(
                Random::randomNum_T<float>(-1000.f, 1000.f),
                Random::randomNum_T<float>(-1000.f, 1000.f),
                Random::randomNum_T<float>(-1000.f, 1000.f),
                0
                );
            mdls_i[i] = ModelInstance(mdls[0], temp, temp);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        LOGGER->success("Loaded ${} instances in ${} seconds!", mdls_i.size(), duration.count());
    }
};

#endif //INC_2G43S_MODELBUS_H