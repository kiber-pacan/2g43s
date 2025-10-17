#include <memory>
#include <vector>

#include "../ModelInstance.hpp"
#include "../ParsedModel.hpp"
#include <omp.h>
#include "ModelBus.hpp"

#include <ranges>

#include "../../util/Random.hpp"

ModelBus::ModelBus() {
    LOGGER = Logger::of("ModelBus");
}


#pragma region parsedModels
template<std::convertible_to<std::string>... T>
void ModelBus::loadModels(const std::string& location, const T&... files) {
    static_assert(sizeof...(files) > 0, "You must provide at least one file to load!");

    (groups_map.insert({files, ModelGroup(std::make_shared<ParsedModel>(location + files))}), ...);
}

template<std::convertible_to<std::string>... T>
void ModelBus::destroyModels(const T&... files) {
    static_assert(sizeof...(files) > 0, "You must provide at least one file to destroy!");

    (groups_map.erase(files), ...);
}

std::shared_ptr<ParsedModel> ModelBus::getModel(const std::string& file) {
    if (const auto it = groups_map.find(file); it != groups_map.end()) {
        return it->second.model;
    }

    return nullptr;
}
#pragma endregion


#pragma region instanceModels
/// TIP args: pos, rot, scl (MAX 3 ELEMENTS)
void ModelBus::createModelInstance(const std::string& file, const auto&... args) {
    static_assert(sizeof...(args) <= 3, "Maximum 3 arguments allowed!");

    if (groups_map.contains(file)) {
        groups_map[file].instances.emplace_back(groups_map[file].model, args...);
    } else {
        LOGGER->error("File ${} does not exist", file);
    }
}
#pragma endregion


#pragma region buffers
VkDeviceSize ModelBus::getIndexBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto &model : std::views::transform(std::views::values(groups_map), &ModelGroup::model)) {
        bufferSize += sizeof(model->indices[0]) * model->indices.size();
    }

    return bufferSize;
}

VkDeviceSize ModelBus::getVertexBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto& model : std::views::transform(std::views::values(groups_map), &ModelGroup::model)) {
        for (const auto& mesh: model->meshes) {
            bufferSize += sizeof(mesh[0]) * mesh.size();
        }
    }

    return bufferSize;
}

VkDeviceSize ModelBus::getModelBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto& instance : std::views::transform(std::views::values(groups_map), &ModelGroup::instances)) {
        bufferSize += sizeof(instance[0]) * instance.size();
    }

    return bufferSize;
}

VkDeviceSize ModelBus::getModelDataBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto& instance : std::views::transform(std::views::values(groups_map), &ModelGroup::instances)) {
        bufferSize += sizeof(glm::vec4) * instance.size() * 4;
    }

    return bufferSize;
}
#pragma endregion


#pragma region data
// Model loading methods
std::vector<uint32_t> ModelBus::getAllIndices() const {
    std::vector<uint32_t> indices{};
    for (const auto& model : std::views::transform(std::views::values(groups_map), &ModelGroup::model)) {
        indices.append_range(model->indices);
    }

    return indices;
}

std::vector<uint32_t> ModelBus::getIndices(const std::string& file) const {
    if (const auto& it = groups_map.find(file); it != groups_map.end()) {
        return it->second.model->indices;
    }
    return {};
}


std::vector<Vertex> ModelBus::getAllVertices() const {
    std::vector<Vertex> vertices{};

    for (const auto& model : std::views::transform(std::views::values(groups_map), &ModelGroup::model)) {
        for (const auto& mesh: model->meshes) {
            vertices.append_range(mesh);
        }
    }

    return vertices;
}

std::vector<Vertex> ModelBus::getVertices(const std::string& file) const {
    std::vector<Vertex> vertices{};

    if (const auto& it = groups_map.find(file); it != groups_map.end()) {
        const auto& meshes = it->second.model->meshes;
        std::for_each(meshes.begin(), meshes.end(), [&vertices](const auto& mesh) {
            vertices.append_range(mesh);
        });
    }

    return vertices;
}
#pragma endregion


#pragma region count
size_t ModelBus::modelsCount() const {
    return groups_map.size();
}


size_t ModelBus::getInstanceCount(const std::string& file) const {
    if (const auto& it = groups_map.find(file); it != groups_map.end()) {
        return it->second.instances.size();
    }
    return 0;
}

size_t ModelBus::getTotalInstanceCount() const {
    const auto& instances = std::views::transform(std::views::values(groups_map), &ModelGroup::instances);

    return std::accumulate(instances.begin(), instances.end(), 0, [](uint32_t sum, const auto& instance) {
        return sum + instance.size();
    });
}


uint32_t ModelBus::getIndexCount(const std::string& name) const {
    return groups_map.at(name).model->indices.size();
}

uint32_t ModelBus::getIndexCount(const std::shared_ptr<ParsedModel>& model) {
    return model->indices.size();
}


int32_t ModelBus::getVertexCount(const std::string& name) const {
    const auto& meshes = groups_map.at(name).model->meshes;

    return std::accumulate(meshes.begin(), meshes.end(), 0, [](uint32_t sum, const auto& mesh) {
        return sum + mesh.size();
    });
}

int32_t ModelBus::getVertexCount(const std::shared_ptr<ParsedModel>& model) {
    return std::accumulate(model->meshes.begin(), model->meshes.end(), 0, [](uint32_t sum, const auto& mesh) {
        return sum + mesh.size();
    });
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
    loadModels("/home/down1/2g43s/core/models/", "cube.glb");

    const auto start = std::chrono::high_resolution_clock::now();
    static constexpr size_t count = 1000000;

    auto& cubeGroup = groups_map["cube.glb"];
    cubeGroup.instances.resize(count);

    #pragma omp parallel for schedule(static) default(none) shared(count, cubeGroup)
    for (int i = 0; i < count; ++i) {
        const glm::vec4 pos(Random::randomNum_T<float>(-1000.f, 1000.f), Random::randomNum_T<float>(-1000.f, 1000.f), Random::randomNum_T<float>(-1000.f, 1000.f), 0);
        const glm::vec4 pos1(Random::randomNum_T<float>(-1.f, 1.f), Random::randomNum_T<float>(-1.f, 1.f), Random::randomNum_T<float>(-1.f, 1.f), 0);


        cubeGroup.instances[i] = ModelInstance(groups_map["cube.glb"].model, pos, pos1);
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;

    LOGGER->success("Loaded ${} instances in ${} seconds!", getTotalInstanceCount(), duration.count());
}
