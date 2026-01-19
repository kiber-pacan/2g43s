#ifndef PARSEDMODEL_H
#define PARSEDMODEL_H

#include <vector>
#include <glm/fwd.hpp>
#include "fastgltf/core.hpp"
#include <glm/ext/quaternion_geometric.hpp>

#include <libs/stb/stb_image.h>
#include "Texture.hpp"
#include "primitives/Vertex.hpp"

class ParsedModel {
    public:
    std::vector<std::vector<Vertex>> meshes{};
    std::vector<glm::uint32_t> indices{};
    glm::vec4 sphere{};

    std::vector<Texture> textures;

    ParsedModel() = default;

    explicit ParsedModel(const std::string& path) {
        loadModel(loadAsset(path));
        calcOcclusionSphere();
    }

    static fastgltf::Asset loadAsset(const std::filesystem::path& path);

    void loadModel(fastgltf::Asset asset);

    void processImageData(fastgltf::Image& image, fastgltf::Asset& asset);


    void calcOcclusionSphere();
};

#endif //PARSEDMODEL_H
