//
// Created by down on 03.10.2025.
//

#ifndef INC_2G43S_MODELBUS_H
#define INC_2G43S_MODELBUS_H
#include <memory>
#include <vector>

#include "ModelInstance.h"
#include "ParsedModel.h"

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
    void createModelInstance(const std::string& model, const glm::vec3 p = glm::vec3(0.0f), const glm::vec3 l = glm::vec3(0.0f), const glm::vec3 s = glm::vec3(1.0f)) {
        for (const std::shared_ptr<ParsedModel> & mdl: mdls) {
            if (mdl->name == model) {
                mdls_i.emplace_back(mdl, p, l, s);
            }
        }
    }

    void createModelInstance(const std::shared_ptr<ParsedModel>& mdl, const glm::vec3 p = glm::vec3(0.0f), const glm::vec3 l = glm::vec3(0.0f), const glm::vec3 s = glm::vec3(1.0f)) {
        mdls_i.emplace_back(mdl, p, l, s);
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

        return indices;
    }

    std::vector<Vertex> getVertices() const {
        std::vector<Vertex> vertices{};
        for (const auto& mdl: mdls) {
            for (const auto& mesh: mdl->meshes) {
                vertices.append_range(mesh);
            }
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
        loadModel("/mnt/sda1/CLionProjects/2g43s/core/models/", "landscape.glb");

        auto start = std::chrono::high_resolution_clock::now();

        for (int x = 0; x < 1000000; ++x) {
            glm::vec3 temp = glm::vec3(Tools::randomNum<float>(-1000.0f, 1000.0f), Tools::randomNum<float>(-1000.0f, 1000.0f), Tools::randomNum<float>(-1000.0f, 1000.0f));
            float rand = Tools::randomNum<float>(-1.0f, 1.0f);
            createModelInstance(
            mdls[0],
            temp,
            temp
            );
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        LOGGER->success("Loaded ${} instances in ${} seconds!", mdls_i.size(), duration.count());
    }
};

#endif //INC_2G43S_MODELBUS_H