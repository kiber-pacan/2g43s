#include <memory>
#include <vector>

#include "../ModelInstance.hpp"
#include "../ParsedModel.hpp"
#include <omp.h>
#include "ModelBus.hpp"
#include "../../util/Random.hpp"

ModelBus::ModelBus() {
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
void ModelBus::loadModel(const std::string& location, const std::string& file) {
    std::filesystem::path path = location + file;
    mdls.emplace_back(std::make_shared<ParsedModel>(path));
}

void ModelBus::loadModels(const std::string& location, const std::vector<std::string>& files) {
    for (const auto& name : files) {
        std::filesystem::path path = location + name;
        mdls.emplace_back(std::make_shared<ParsedModel>(path));
    }
}


void ModelBus::unloadModel(const std::string& file) {
    for (int i = 0; i < mdls.size(); ++i) {
        if (mdls[i]->name == file) {
            mdls.erase(mdls.begin() + i);
        }
    }
}

void ModelBus::unloadModels(const std::vector<std::string>& files) {
    for (int i = 0; i < mdls.size(); ++i) {
        for (const std::string& file : files) {
            if (mdls[i]->name == file) {
                mdls.erase(mdls.begin() + i);
            }
        }
    }
}

std::shared_ptr<ParsedModel> ModelBus::getModel(const std::string& name) const {
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
void ModelBus::addModelInstance(const std::string& model, const glm::vec4 p, const glm::vec4 r , const glm::vec4 s) {
    for (const std::shared_ptr<ParsedModel> & mdl: mdls) {
        if (mdl->name == model) {
            mdls_i.emplace_back(mdl, p, r, s);
        }
    }
}

void ModelBus::addModelInstance(const std::shared_ptr<ParsedModel>& mdl, const glm::vec4 p, const glm::vec4 r, const glm::vec4 s) {
    mdls_i.emplace_back(mdl, p, r, s);
}

ModelInstance ModelBus::createModelInstance(const std::shared_ptr<ParsedModel>& mdl, const glm::vec4 p, const glm::vec4 r, const glm::vec4 s) {
    return ModelInstance(mdl, p, r, s);
}

void ModelBus::destroyModelInstance(const std::string& name) {
    /*for (int i = 0; i < mdls_i.size(); ++i) {
        if (mdls_i[i].name == name) {
            mdls_i.erase(mdls_i.begin() + i);
        }
    }*/
}


#pragma endregion


#pragma region buffers
VkDeviceSize ModelBus::getIndexBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto& mdl: mdls) {
        bufferSize += sizeof(mdl->indices[0]) * mdl->indices.size();
    }

    return bufferSize;
}

VkDeviceSize ModelBus::getVertexBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto& mdl: mdls) {
        for (const auto& mesh: mdl->meshes) {
            bufferSize += sizeof(mesh[0]) * mesh.size();
        }
    }

    return bufferSize;
}


std::vector<uint32_t> ModelBus::getIndices() const {
    std::vector<uint32_t> indices{};
    for (const auto& mdl: mdls) {
        indices.append_range(mdl->indices);
    }

    for (uint32_t index : indices) {
        //std::cout << index << std::endl;
    }

    return indices;
}

std::vector<Vertex> ModelBus::getVertices() const {
    std::vector<Vertex> vertices{};
    for (const auto& mdl: mdls) {
        for (const auto& mesh: mdl->meshes) {
            vertices.append_range(mesh);
        }
    }

    for (Vertex vertex : vertices) {
        //std::cout << vertex.pos.x << " " << vertex.pos.y << " " << vertex.pos.z << std::endl;
    }

    return vertices;
}

#pragma endregion


#pragma region helper
size_t ModelBus::modelsCount() const {
    return mdls.size();
}

size_t ModelBus::getInstanceCount(const std::shared_ptr<ParsedModel>& model) const {
    size_t instanceCount = 0;
    for (const auto& mdl: mdls_i) {
        instanceCount += mdl.mdl == model ? 1 : 0;
    }

    return instanceCount;
}

uint32_t ModelBus::getIndexCount(const std::string& name) const {
    if (const std::shared_ptr<ParsedModel> model = getModel(name); model != nullptr) {
        return model->indices.size();
    }

    return 0;
}

uint32_t ModelBus::getIndexCount(const std::shared_ptr<ParsedModel>& model) {
    return model->indices.size();
}

uint32_t ModelBus::getVertexCount(const std::string& name) const {
    uint32_t vertexCount = 0;
    if (const std::shared_ptr<ParsedModel> model = getModel(name); model != nullptr) {
        for (const auto& mesh: model->meshes) {
            vertexCount += mesh.size();
        }
    }

    return vertexCount;
}

int32_t ModelBus::getVertexCount(const std::shared_ptr<ParsedModel>& model) {
    int32_t vertexCount = 0;
    for (const auto& mesh: model->meshes) {
        vertexCount += mesh.size();
    }

    return vertexCount;
}

#pragma endregion


#pragma region commands
void ModelBus::addCommands(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t  vertexOffset, uint32_t firstInstance) {
    VkDrawIndexedIndirectCommand drawCommand{};
    drawCommand.indexCount = indexCount;
    drawCommand.instanceCount = instanceCount;
    drawCommand.firstIndex = firstIndex;
    drawCommand.vertexOffset = vertexOffset;
    drawCommand.firstInstance = firstInstance;

    drawCommands.emplace_back(drawCommand);
}
#pragma endregion


void ModelBus::test() {
    loadModel("/home/down1/2g43s/core/models/", "cube_1_material.glb");
    loadModel("/home/down1/2g43s/core/models/", "landscape.glb");

    auto start = std::chrono::high_resolution_clock::now();
    static size_t count = 1000000;
    mdls_i.resize(count);

#pragma omp single
    //std::cout << "Threads: " << omp_get_num_threads() << std::endl;

#pragma omp parallel for default(none) shared(mdls, mdls_i, count)
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
