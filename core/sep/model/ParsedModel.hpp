#ifndef PARSEDMODEL_H
#define PARSEDMODEL_H
#include <ranges>
#include <vector>
#include <glm/ext/quaternion_geometric.hpp>

#include "primitives/Vertex.hpp"
#include "../util/Logger.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "primitives/Sphere.hpp"

class ParsedModel {
    public:
    std::vector<std::vector<Vertex>> meshes{};
    std::vector<uint32_t> indices{};
    glm::vec4 sphere{};

    ParsedModel() = default;

    explicit ParsedModel(const std::string& path) {
        loadModel(loadAsset(path));
        calcOcclusionSphere();
    }

    static fastgltf::Asset loadAsset(const std::filesystem::path& path) {
        Logger* LOGGER = Logger::of("ParsedModel.h");

        constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices;
        static constexpr auto extensions = fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_texture_transform | fastgltf::Extensions::KHR_materials_variants | fastgltf::Extensions::KHR_materials_specular;

        fastgltf::Parser parser(extensions);
        auto buffer = fastgltf::GltfDataBuffer::FromPath(path);

        if (!static_cast<bool>(buffer)) {
            LOGGER->error("Failed to open glTF file: ${}", fastgltf::getErrorMessage(buffer.error()));
        }

        auto asset = parser.loadGltf(buffer.get(), path.parent_path(), options);

        if (asset.error() != fastgltf::Error::None) {
            LOGGER->error("Error loading model: ${}", fastgltf::getErrorMessage(asset.error()));
        }

        return std::move(asset.get());
    }

    void loadModel(fastgltf::Asset asset) {
        for (fastgltf::Mesh& mesh : asset.meshes) {
            for (fastgltf::Primitive& primitive : mesh.primitives) {
                auto* position = primitive.findAttribute("POSITION");
                auto& positionAccessor = asset.accessors[position->accessorIndex];

                std::vector<Vertex> tempMesh(positionAccessor.count);
                std::vector<std::uint32_t> tempIndices{};

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                    tempMesh[idx] = {
                        {pos[0], pos[1], pos[2]},
                        {1, 1, 1},
                        {fastgltf::math::fvec2()[0],fastgltf::math::fvec2()[1]}
                    };
                });

                auto* texcoord = primitive.findAttribute("TEXCOORD_0");
                if (texcoord != nullptr) {
                    auto& texcoordAccessor = asset.accessors[texcoord->accessorIndex];

                    if (texcoordAccessor.type == fastgltf::AccessorType::Vec2) {
                        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                            asset, texcoordAccessor,
                            [&](fastgltf::math::fvec2 uv, std::size_t idx) {
                                tempMesh[idx].texCoord = { uv[0], uv[1] };
                            }
                        );
                    }
                }

                uint32_t baseVertex = 0;
                for (auto& m : this->meshes) {
                    baseVertex += static_cast<uint32_t>(m.size());
                }

                this->meshes.emplace_back(std::move(tempMesh));

                if (primitive.indicesAccessor.has_value()) {
                    auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
                    tempIndices.resize(accessor.count);

                    std::size_t idx = 0;
                    fastgltf::iterateAccessor<std::uint32_t>(asset, accessor, [&](std::uint32_t index) {
                        tempIndices[idx++] = index + baseVertex;
                    });
                }


                this->indices.append_range(tempIndices);
            }
        }
    }


    void calcOcclusionSphere() {
        std::vector<Vertex> vertices; vertices.append_range(std::views::join(meshes));

        // Center
        for (const Vertex& p : vertices) {
            sphere.x += p.pos.x;
            sphere.y += p.pos.y;
            sphere.z += p.pos.z;
        }

        sphere /= static_cast<float>(vertices.size());

        // Radius
        for (const Vertex& p : vertices) {
            glm::vec3 vec = glm::vec3(sphere.x, sphere.y, sphere.z);
            sphere.w = std::max(sphere.w, glm::length(p.pos - vec));
        }

        std::cout << sphere.x << " " << sphere.y << " " << sphere.z << std::endl;
        std::cout << sphere.w << std::endl;
    }
};

#endif //PARSEDMODEL_H
