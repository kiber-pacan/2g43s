#include <memory>
#include <vector>

#include "ModelBus.hpp"

#include <ranges>

#include "Images.hpp"

ModelBus::ModelBus() : LOGGER(Logger("ModelBus")) {
}


#pragma region parsedModels
template<std::convertible_to<std::string>... T>
void ModelBus::loadModels(const std::string& location, const T&... files) {
    static_assert(sizeof...(files) > 0, "You must provide at least one file to load!");
    ((std::cout << files << std::endl), ...);
    (groups_map.insert({files, ModelGroup(std::make_shared<ParsedModel>(location + files))}),...);
}

void ModelBus::loadModelTextures(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkPhysicalDevice& physicalDevice, VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory) {
    int globalIndex = 0;
    for (const auto& model : std::views::transform(groups_map | std::views::values, &ModelGroup::model)) {
        for (auto & texture : model->textures) {
            Images::createTextureImage(device, commandPool, graphicsQueue, physicalDevice, stagingBuffer, stagingBufferMemory, texture);
            Images::createTextureImageView(device, texture.textureImage, texture.textureImageView);

            texture.index = globalIndex;
            globalIndex++;
        }
    }
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
void ModelBus::instance(const std::string& file, const auto&... args) {
    static_assert(sizeof...(args) <= 3, "Maximum 3 arguments allowed!");

    if (groups_map.contains(file)) {
        groups_map[file].instances.emplace_back(groups_map[file].model, args...);
    } else {
        LOGGER.error("File ${} does not exist", file);
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
        bufferSize += sizeof(glm::vec4) * instance.size() * 3;
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

size_t ModelBus::getTotalModelCount() const {
    return groups_map.size();
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

void ModelBus::randomVolume(size_t count, const std::string& file, const float min, const float max) {
    randomVolume(count, file, glm::vec3(min), glm::vec3(max));
}

void ModelBus::randomVolume(size_t count, const std::string& file, const glm::vec3& min, const glm::vec3& max) {
    auto& group = groups_map[file];
    group.instances.resize(count);

    #pragma omp parallel for schedule(static) default(none) shared(count, group, min, max)
    for (int x = 0; x < count; ++x) {
        const glm::vec4 pos(Random::randomNum_T(min.x, max.x), Random::randomNum_T(min.y, max.y), Random::randomNum_T(min.y, max.z), 0);

        group.instances[x] = ModelInstance(group.model, pos);

    }
}

void ModelBus::square(size_t count, const std::string& file, const double gap) {
    auto& group = groups_map[file];
    group.instances.resize(count * count);

    #pragma omp parallel for schedule(static) default(none) shared(count, group, gap)
    for (int x = 0; x < count; ++x) {
        for (int y = 0; y < count; ++y) {
            const glm::vec4 pos(x * gap, y * gap, 0, 0);

            group.instances[x * count + y] = ModelInstance(group.model, pos);
        }
    }
}

void ModelBus::loadModels() {
    const auto start1 = std::chrono::high_resolution_clock::now();
    loadModels("/home/down1/2g43s/core/models/", "land2.glb");
    const auto end1 = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration1 = end1 - start1;

    LOGGER.success("Loaded ${} models in ${} seconds!", getTotalModelCount(), duration1.count());

    const auto start = std::chrono::high_resolution_clock::now();

    //randomVolume(1000000, "box.glb", -1000, 1000);
    instance("land2.glb");

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;

    LOGGER.success("Loaded ${} instances in ${} seconds!", getTotalInstanceCount(), duration.count());
}
