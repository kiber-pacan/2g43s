//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_SHADER_H
#define INC_2G43S_SHADER_H

#include <filesystem>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <filesystem>
#include <stdexcept>
#include <shaderc/shaderc.hpp>
#include <nlohmann/json.hpp>




struct Shaders {
    static VkShaderModule createShaderModule(const std::vector<char>& code, const VkDevice& device);

    static std::vector<std::filesystem::path> getShaders();

    static std::vector<std::filesystem::path> getShadersToCompile();

    static void saveShaderToFile(const std::string& path, const std::vector<uint32_t>& spirv);

    static std::vector<uint32_t> compileShader(const std::filesystem::path& file);

    static nlohmann::json createCache(const std::vector<std::filesystem::path>& shaders);

    static nlohmann::json addCacheEntry(std::filesystem::path shader);

};


#endif //INC_2G43S_SHADER_H