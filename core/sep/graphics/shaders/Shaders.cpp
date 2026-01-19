//
// Created by down1 on 29.12.2025.
//

#include "Shaders.hpp"

#include "Tools.hpp"

VkShaderModule Shaders::createShaderModule(const std::vector<char>& code, const VkDevice& device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

std::vector<std::filesystem::path> Shaders::getShaders() {
    std::vector<std::filesystem::path> shaders{};
    std::filesystem::recursive_directory_iterator it(Tools::getShaderPath());

    for (const auto& entry : it) {
        // Skip already compiled shaders
        std::filesystem::path path(entry.path());
        if (path.string().contains("compiled") || !Tools::isShader(path.extension())) continue;

        shaders.emplace_back(path);
    }

    return shaders;
}

void Shaders::saveShaderToFile(const std::string& path, const std::vector<uint32_t>& spirv) {
    if (spirv.empty()) return;

    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + path);
    }

    file.write(reinterpret_cast<const char*>(spirv.data()), spirv.size() * sizeof(uint32_t));

    file.close();
}


#pragma region Compilation
std::vector<std::filesystem::path> Shaders::getShadersToCompile() {
    std::vector<std::filesystem::path> shaders = getShaders();
    std::vector<std::filesystem::path> dirtyShaders{};
    std::ifstream file(Tools::getShaderPath() + "cache.json");
    nlohmann::json cache;

    if (!file.fail()) {
        if (nlohmann::json::accept(file)) {
            file.seekg(0);
            cache = nlohmann::json::parse(file);
        } else {
            createCache(shaders);
        }
    } else {
        cache = createCache(shaders);
    }


    for (const auto& entry : shaders) {
        auto time = std::chrono::file_clock::to_sys(std::filesystem::last_write_time(entry)).time_since_epoch().count();
        auto cacheTime = cache[entry.lexically_relative(Tools::getShaderPath()).string()];
        if (cacheTime != time) {
            dirtyShaders.emplace_back(entry.string());
        }
    }

    return dirtyShaders;
}


std::vector<uint32_t> Shaders::compileShader(const std::filesystem::path& file) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);

    std::ifstream stream{file};
    std::stringstream buffer;
    buffer << stream.rdbuf();

    std::string source = buffer.str();
    std::filesystem::path filename = file.lexically_relative(Tools::getShaderPath());

    std::vector<uint32_t> shader{};

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
        source, Tools::getShaderKind(file.extension()), filename.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "Shader Compilation Error: " << result.GetErrorMessage() << std::endl;
    } else {
        std::cout << "Compiled shader: " << filename << std::endl;

        addCacheEntry(file);
#if __cpp_lib_containers_ranges >= 202202L
        shader.append_range(result);
#else
        shader.insert(shader.end(), std::ranges::begin(result), std::ranges::end(result));
#endif
    }

    return shader;
}
#pragma endregion


#pragma region Cache
nlohmann::json Shaders::createCache(const std::vector<std::filesystem::path>& shaders) {
    nlohmann::json cache;
    for (const auto& shader : shaders) {
        auto time = std::filesystem::last_write_time(shader);
        auto systemTime = std::chrono::file_clock::to_sys(time);

        cache[shader.lexically_relative(Tools::getShaderPath())] = systemTime.time_since_epoch().count();
    }

    std::ofstream file(Tools::getShaderPath() + "cache.json");
    file << cache.dump(0);

    file.close();

    return cache;
}

nlohmann::json Shaders::addCacheEntry(std::filesystem::path shader) {
    std::ifstream file(Tools::getShaderPath() + "cache.json");
    nlohmann::json cache = nlohmann::json::parse(file);

    auto time = std::filesystem::last_write_time(shader);
    auto systemTime = std::chrono::file_clock::to_sys(time);

    cache[shader.lexically_relative(Tools::getShaderPath())] = systemTime.time_since_epoch().count();

    std::ofstream file1(Tools::getShaderPath() + "cache.json");
    file1 << cache.dump(0);

    file1.close();

    return cache;
}
#pragma endregion
