#ifndef TOOLS_H
#define TOOLS_H

#include <random>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <shaderc/shaderc.h>
#include <unordered_map>

// Some tools
class Tools {
    public:
    // For reading shader files
    static std::vector<char> readFile(const char* filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + std::string(filename) + "!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    static std::string getCompiledShaderFilePath(const std::string& shaderFile) {
        std::string path = getCompiledShaderPath() + shaderFile;
        return path;
    }

    static std::string getCompiledShaderPath() {
        return std::string{PROJECT_ROOT} + "core/shaders/compiled/";
    }

    static std::string getShaderFilePath(const std::string& shaderFile) {
        std::string path = getShaderPath() + shaderFile;
        return path;
    }

    static std::string getShaderPath() {
        return std::string{PROJECT_ROOT} + "core/shaders/";
    }

    inline static const std::unordered_map<std::string, shaderc_shader_kind> extensions{
        // Classics
        {".vert", shaderc_glsl_vertex_shader},
        {".frag", shaderc_glsl_fragment_shader},
        {".comp", shaderc_glsl_compute_shader},
        {".geom", shaderc_glsl_geometry_shader},
        {".tesc", shaderc_glsl_tess_control_shader},
        {".tese", shaderc_glsl_tess_evaluation_shader},

        // Ray Tracing (Vulkan KHR / NV)
        {".rgen",  shaderc_glsl_raygen_shader},
        {".rahit", shaderc_glsl_anyhit_shader},
        {".rchit", shaderc_glsl_closesthit_shader},
        {".rmiss", shaderc_glsl_miss_shader},
        {".rint",  shaderc_glsl_intersection_shader},
        {".rcall", shaderc_glsl_callable_shader},

        // Mesh Shaders
        {".task", shaderc_glsl_task_shader},
        {".mesh", shaderc_glsl_mesh_shader}
    };

    static bool isShader(const std::string &extension) {
        return extensions.contains(extension);
    }

    static shaderc_shader_kind getShaderKind(const std::string &extension) {
        const auto it = extensions.find(extension);

        if (it != extensions.end()) {
            return it->second;
        }

        return shaderc_glsl_vertex_shader;
    }

    static std::string getCorePath() {
        return std::string{PROJECT_ROOT} + "core/";
    }
};

#endif //TOOLS_H
