#ifndef PARSEDMODEL_H
#define PARSEDMODEL_H

#include <vector>
#include <numeric>
#include <ranges>

#include <glm/fwd.hpp>
#include "fastgltf/core.hpp"
#include <glm/ext/quaternion_geometric.hpp>
#include <basisu_transcoder.h>

#include "../images/Images.hpp"
#include "Logger.hpp"
#include "fastgltf/tools.hpp"
#include "../images/Texture.hpp"
#include "Vertex.hpp"

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include "../../../cmake-build-debug/_deps/jolt_physics-src/Jolt/Physics/Collision/Shape/MeshShape.h"

class ParsedModel {
    public:
    std::vector<std::vector<Vertex>> meshes{};
    std::vector<glm::uint32_t> indices{};
    glm::vec4 sphere{};

    std::vector<Texture> textures{};

    ParsedModel() = default;

    explicit ParsedModel(const std::string& path) {
        loadModel(path);
        calcOcclusionSphere();
    }

    void loadModel(const std::filesystem::path& path);

    void processImageData(fastgltf::Image& image, fastgltf::Asset& asset, size_t index, size_t textureIndex);


    void calcOcclusionSphere();

    [[nodiscard]] JPH::ShapeRefC createJoltMesh() const;

    JPH::ShapeRefC  createJoltConvexHull() const;
};

#endif //PARSEDMODEL_H
