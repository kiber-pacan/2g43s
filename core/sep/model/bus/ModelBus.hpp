#ifndef INC_2G43S_MODELBUS_HPP
#define INC_2G43S_MODELBUS_HPP

#include <memory>
#include <ranges>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "Logger.hpp"
#include "ModelGroup.h"
#include "ModelInstance.hpp"

class ParsedModel;

struct ModelBus {
    ModelBus() = default;

    #pragma region Variables
    constexpr static uint32_t MAX_MODELS = 4294967295;
    bool dirtyCommands = true;
    #pragma endregion


    #pragma region parsedModels

    template<std::convertible_to<std::string>... T>
    static void loadModels(std::unordered_map<std::string, ModelGroup>& groups, const std::string& location, const T&... files) {
        static_assert(sizeof...(files) > 0, "You must provide at least one file to load!");
        ((std::cout << files << std::endl), ...);
        (groups.insert({files, ModelGroup(std::make_shared<ParsedModel>(location + files))}),...);
    }

    template<std::convertible_to<std::string>... T>
    static void destroyModels(std::unordered_map<std::string, ModelGroup>& groups, const T&... files) {
        static_assert(sizeof...(files) > 0, "You must provide at least one file to destroy!");

        (groups.erase(files), ...);
    }

    static void loadModelTextures(std::unordered_map<std::string, ModelGroup>& groups, const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkPhysicalDevice& physicalDevice, VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory);

    static std::shared_ptr<ParsedModel> getModel(std::unordered_map<std::string, ModelGroup>& groups, const std::string& file);
    #pragma endregion

    #pragma region instanceModels

    #pragma endregion
};

#endif //INC_2G43S_MODELBUS_HPP