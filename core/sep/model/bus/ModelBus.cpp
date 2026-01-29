#include "ModelBus.hpp"

#include "ParsedModel.hpp"

#pragma region parsedModels
void ModelBus::loadModelTextures(std::unordered_map<std::string, ModelGroup>& groups, const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkPhysicalDevice& physicalDevice, VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory) {
    int globalIndex = 0;
    for (const auto& model : std::views::transform(groups | std::views::values, &ModelGroup::model)) {
        for (auto & texture : model->textures) {
            Images::createTextureImage(device, commandPool, graphicsQueue, physicalDevice, stagingBuffer, stagingBufferMemory, texture);
            Images::createTextureImageView(device, texture.textureImage, texture.textureImageView, VK_FORMAT_BC7_UNORM_BLOCK);

            texture.index = globalIndex;
            globalIndex++;
        }
    }
}

std::shared_ptr<ParsedModel> ModelBus::getModel(std::unordered_map<std::string, ModelGroup>& groups, const std::string& file) {
    if (const auto it = groups.find(file); it != groups.end()) {
        return it->second.model;
    }

    return nullptr;
}
#pragma endregion



